#include "scheduler/Scheduler.hpp"
#include "common/Convertors.hpp"
#include "common/Logger.hpp"
#include "scheduler/strategies/StrategyBatch.hpp"
#include "scheduler/strategies/StrategyFIFO.hpp"

#include <utility>

namespace IP::scheduler
{
Scheduler::Scheduler(std::shared_ptr<backends::BackendManager> backends,
    std::shared_ptr<model_manager::ModelManager> model_manager, std::shared_ptr<core::OpRepository> op_repository,
    std::shared_ptr<IPolicySelector> policy_selector)
    : backends_(std::move(backends))
    , model_manager_(std::move(model_manager))
    , op_repository_(std::move(op_repository))
    , policy_selector_(std::move(policy_selector))
    , stop_requested_(false)
    , inflight_task_count_(0)
    , logger_("Scheduler: ")
    , thread_pool_(std::make_shared<common::ThreadPool::ThreadPool>())
    , worker_count_(std::max<size_t>(1, std::thread::hardware_concurrency()))
{
    if (policy_selector_ == nullptr)
    {
        policy_selector_ = std::make_shared<DefaultPolicySelector>();
    }
    init();
}

Scheduler::~Scheduler()
{
    stop();
}

void Scheduler::init()
{
    std::lock_guard<std::mutex> lock(strategies_mutex_);
    strategies_.emplace(common::defs::ScheduleStrategy::FIFO, std::make_shared<StrategyFIFO>());
    strategies_.emplace(common::defs::ScheduleStrategy::MaxThroughput, std::make_shared<StrategyBatch>());
    dispatcher_thread_ = std::thread(&Scheduler::dispatchLoop, this);
}

bool Scheduler::admit(std::shared_ptr<common::defs::Task> task)
{
    if (task == nullptr || stop_requested_.load())
    {
        TRC_WARNING << logger_ << "Scheduler is stopping, cannot schedule new tasks.";
        return false;
    }

    auto context = buildAdmissionContext(task->modelName());
    task->strategy = policy_selector_->selectStrategy(context);
    auto scheduler_strategy = strategy(task->strategy);
    if (scheduler_strategy == nullptr)
    {
        TRC_WARNING << logger_ << "Unsupported scheduling strategy for request " << task->requestId();
        return false;
    }

    if (context.model_meta.conf != nullptr && context.model_meta.conf->max_queue_size > 0 &&
        scheduler_strategy->pendingTaskCount() >= context.model_meta.conf->max_queue_size)
    {
        if (context.model_meta.conf->drop_if_queue_full)
        {
            task->response.error_message = "scheduler queue is full";
            updateOperation(
                task->requestId(), common::defs::OperationState::FAILURE, task->response, task->response.error_message);
            return false;
        }
    }

    trackTask(task);

    if (!scheduler_strategy->admit(task, context))
    {
        forgetTask(task->requestId());
        return false;
    }

    admitModelStat(task->modelName());
    strategies_cv_.notify_one();
    return true;
}

bool Scheduler::submitTask(common::defs::RequestInfo request)
{
    auto task = std::make_shared<common::defs::Task>(std::move(request));
    return admit(std::move(task));
}

bool Scheduler::cancelTask(const std::string& pre_request_id)
{
    auto [found, op_info] = op_repository_->getOperation(pre_request_id);
    if (!found)
    {
        return false;
    }

    if (op_info.state == common::defs::OperationState::CANCELED)
    {
        return true;
    }

    if (op_info.state == common::defs::OperationState::SUCCESS ||
        op_info.state == common::defs::OperationState::FAILURE)
    {
        return false;
    }

    auto task = trackedTask(pre_request_id);
    if (task == nullptr)
    {
        return false;
    }

    if (task->cancel_requested != nullptr)
    {
        task->cancel_requested->store(true);
    }

    bool revoked = false;
    {
        std::lock_guard<std::mutex> lock(strategies_mutex_);
        for (const auto& [kind, scheduler_strategy] : strategies_)
        {
            (void)kind;
            if (scheduler_strategy != nullptr && scheduler_strategy->revoke(pre_request_id))
            {
                revoked = true;
                break;
            }
        }
    }

    if (revoked)
    {
        finalizeCanceledTask(task, "request canceled before execution");
        return true;
    }

    updateOperation(pre_request_id, common::defs::OperationState::CANCEL_REQUESTED, task->response,
        "cancel requested; waiting for in-flight execution to finish");
    return true;
}

void Scheduler::stop()
{
    if (stop_requested_.exchange(true))
    {
        return;
    }

    strategies_cv_.notify_all();
    if (dispatcher_thread_.joinable())
    {
        dispatcher_thread_.join();
    }
    thread_pool_->stop();
}

common::defs::SchedulerRuntimeSnapshot Scheduler::runtimeSnapshot() const
{
    common::defs::SchedulerRuntimeSnapshot snapshot;
    snapshot.worker_count = worker_count_;
    snapshot.inflight_task_count = inflight_task_count_.load();
    snapshot.queued_task_count = queuedTaskCount();
    snapshot.stop_requested = stop_requested_.load();

    // Aggregate per-model stats collected by Scheduler
    {
        std::lock_guard<std::mutex> lock(model_stats_mutex_);
        for (const auto& [name, s] : model_stats_)
        {
            common::defs::SchedulerRuntimeSnapshot::ModelStats ms;
            ms.model_name = name;
            ms.total_admitted = s.total_admitted;
            ms.total_succeeded = s.total_succeeded;
            ms.total_failed = s.total_failed;
            ms.total_canceled = s.total_canceled;
            ms.current_inflight = s.current_inflight;
            snapshot.model_stats[name] = std::move(ms);
        }
    }

    // Aggregate batch window states from StrategyBatch
    {
        std::lock_guard<std::mutex> lock(strategies_mutex_);
        for (const auto& [kind, strat] : strategies_)
        {
            (void)kind;
            if (strat == nullptr)
                continue;
            for (auto& ws : strat->batchWindowStates())
                snapshot.batch_windows[ws.model_name] = std::move(ws);
        }
    }

    return snapshot;
}

void Scheduler::dispatchLoop()
{
    while (true)
    {
        std::unique_lock<std::mutex> lock(strategies_mutex_);

        // Sleep for exactly as long as needed:
        //   - FIFO has tasks  → nextReadyIn() == 0ms → returns immediately
        //   - Batch waiting   → nextReadyIn() == remaining timeout
        //   - All queues empty → nextReadyIn() == 24h (woken by notify on admit)
        // Predicate only checks stop_requested_ so that a notify_one() from
        // admit() wakes us early but we don't spin when batch tasks are not
        // yet ready.
        auto wait_dur = computeMinWaitDuration();
        strategies_cv_.wait_for(lock, wait_dur,
            [this]()
            {
                return stop_requested_.load();
            });

        if (stop_requested_.load() && queuedTaskCount() == 0)
        {
            break;
        }

        auto unit = selectReadyUnit();
        lock.unlock();

        if (!unit.has_value() || unit->empty())
        {
            continue;
        }

        auto ready_unit = std::move(*unit);
        inflight_task_count_.fetch_add(1);
        if (!thread_pool_->enqueue(
                [this, unit = ready_unit]() mutable
                {
                    execute(std::move(unit));
                }))
        {
            inflight_task_count_.fetch_sub(1);
            for (auto& task : ready_unit.tasks)
            {
                if (task == nullptr)
                {
                    continue;
                }
                task->response.error_message = "failed to dispatch task to worker pool";
                updateOperation(task->requestId(), common::defs::OperationState::FAILURE, task->response,
                    task->response.error_message);
                forgetTask(task->requestId());
            }
        }
    }
}

std::optional<common::defs::DispatchUnit> Scheduler::selectReadyUnit()
{
    for (const auto& [kind, scheduler_strategy] : strategies_)
    {
        (void)kind;
        if (scheduler_strategy == nullptr)
        {
            continue;
        }

        auto unit = scheduler_strategy->select(buildSelectionContext());
        if (unit.has_value() && !unit->empty())
        {
            return unit;
        }
    }

    return std::nullopt;
}

void Scheduler::execute(common::defs::DispatchUnit unit)
{
    struct InflightGuard
    {
        std::atomic<size_t>& counter;
        ~InflightGuard()
        {
            counter.fetch_sub(1);
        }
    } guard{ inflight_task_count_ };

    for (auto& task : unit.tasks)
    {
        if (task == nullptr)
        {
            continue;
        }

        if (task->cancel_requested != nullptr && task->cancel_requested->load())
        {
            finalizeCanceledTask(task, "request canceled before execution");
            recordModelResult(task->modelName(), common::defs::OperationState::CANCELED);
            continue;
        }

        updateOperation(task->requestId(), common::defs::OperationState::ONGOING, task->response);

        auto model_meta = model_manager_->resolveModel(task->modelName());
        if (model_meta.conf == nullptr)
        {
            task->response.error_message = "model not found";
            updateOperation(
                task->requestId(), common::defs::OperationState::FAILURE, task->response, task->response.error_message);
            forgetTask(task->requestId());
            recordModelResult(task->modelName(), common::defs::OperationState::FAILURE);
            continue;
        }

        if (model_meta.state != common::defs::ModelState::ENABLED)
        {
            task->response.error_message = "model is not enabled";
            updateOperation(
                task->requestId(), common::defs::OperationState::FAILURE, task->response, task->response.error_message);
            forgetTask(task->requestId());
            recordModelResult(task->modelName(), common::defs::OperationState::FAILURE);
            continue;
        }

        auto backend = backends_->getBackend(common::defs::backendTypeFromString(model_meta.conf->backend_type));
        if (backend == nullptr)
        {
            task->response.error_message = "backend not found";
            updateOperation(
                task->requestId(), common::defs::OperationState::FAILURE, task->response, task->response.error_message);
            forgetTask(task->requestId());
            recordModelResult(task->modelName(), common::defs::OperationState::FAILURE);
            continue;
        }

        if (!backend->infer(task->request, task->response))
        {
            if (task->cancel_requested != nullptr && task->cancel_requested->load())
            {
                finalizeCanceledTask(task, "request canceled during execution");
                recordModelResult(task->modelName(), common::defs::OperationState::CANCELED);
                continue;
            }
            if (!task->response.error_message.has_value())
            {
                task->response.error_message = "inference execution failed";
            }
            updateOperation(
                task->requestId(), common::defs::OperationState::FAILURE, task->response, task->response.error_message);
            forgetTask(task->requestId());
            recordModelResult(task->modelName(), common::defs::OperationState::FAILURE);
            continue;
        }

        if (task->cancel_requested != nullptr && task->cancel_requested->load())
        {
            finalizeCanceledTask(task, "request canceled during execution");
            recordModelResult(task->modelName(), common::defs::OperationState::CANCELED);
            continue;
        }

        updateOperation(task->requestId(), common::defs::OperationState::SUCCESS, task->response);
        forgetTask(task->requestId());
        recordModelResult(task->modelName(), common::defs::OperationState::SUCCESS);
    }
}

void Scheduler::updateOperation(const std::string& request_id, common::defs::OperationState new_state,
    const common::defs::ResponseInfo& response, std::optional<std::string> message)
{
    common::defs::OperationInfo op_info;
    auto [found, existing] = op_repository_->getOperation(request_id);
    if (found)
    {
        op_info = std::move(existing);
    }

    op_info.id = request_id;
    op_info.type = common::defs::OperationType::SUBMIT_INFERENCE;
    op_info.state = new_state;
    op_info.response = response;
    op_info.message = std::move(message);
    op_repository_->updateOperation(request_id, op_info);
}

void Scheduler::trackTask(const std::shared_ptr<common::defs::Task>& task)
{
    if (task == nullptr)
    {
        return;
    }

    std::lock_guard<std::mutex> lock(tracked_tasks_mutex_);
    tracked_tasks_[task->requestId()] = task;
}

std::shared_ptr<common::defs::Task> Scheduler::trackedTask(const std::string& request_id) const
{
    std::lock_guard<std::mutex> lock(tracked_tasks_mutex_);
    auto iter = tracked_tasks_.find(request_id);
    if (iter == tracked_tasks_.end())
    {
        return nullptr;
    }
    return iter->second;
}

void Scheduler::forgetTask(const std::string& request_id)
{
    std::lock_guard<std::mutex> lock(tracked_tasks_mutex_);
    tracked_tasks_.erase(request_id);
}

void Scheduler::finalizeCanceledTask(
    const std::shared_ptr<common::defs::Task>& task, std::optional<std::string> message)
{
    if (task == nullptr)
    {
        return;
    }

    task->response.error_message = "request canceled";
    updateOperation(task->requestId(), common::defs::OperationState::CANCELED, task->response,
        message.has_value() ? std::move(message) : std::optional<std::string>{ "request canceled" });
    forgetTask(task->requestId());
}

SchedulingContext Scheduler::buildAdmissionContext(const std::string& model_name) const
{
    SchedulingContext context;
    if (!model_name.empty())
    {
        context.model_meta = model_manager_->resolveModel(model_name);
    }
    context.runtime_snapshot = runtimeSnapshot();
    return context;
}

SelectionContext Scheduler::buildSelectionContext() const
{
    SelectionContext context;
    context.runtime_snapshot = runtimeSnapshot();
    return context;
}

std::shared_ptr<IStrategy> Scheduler::strategy(common::defs::ScheduleStrategy kind) const
{
    std::lock_guard<std::mutex> lock(strategies_mutex_);
    auto iter = strategies_.find(kind);
    if (iter != strategies_.end() && iter->second != nullptr)
    {
        return iter->second;
    }

    auto fallback = strategies_.find(common::defs::ScheduleStrategy::FIFO);
    return fallback == strategies_.end() ? nullptr : fallback->second;
}

size_t Scheduler::queuedTaskCount() const
{
    size_t total = 0;
    for (const auto& [kind, scheduler_strategy] : strategies_)
    {
        (void)kind;
        if (scheduler_strategy != nullptr)
        {
            total += scheduler_strategy->pendingTaskCount();
        }
    }
    return total;
}

std::chrono::milliseconds Scheduler::computeMinWaitDuration() const
{
    // strategies_mutex_ must already be held by the caller (dispatchLoop).
    auto min_dur = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::hours{ 24 });
    for (const auto& [kind, strat] : strategies_)
    {
        (void)kind;
        if (strat != nullptr)
            min_dur = std::min(min_dur, strat->nextReadyIn());
    }
    return min_dur;
}

void Scheduler::admitModelStat(const std::string& model_name)
{
    std::lock_guard<std::mutex> lock(model_stats_mutex_);
    auto& s = model_stats_[model_name];
    ++s.total_admitted;
    ++s.current_inflight;
}

void Scheduler::recordModelResult(const std::string& model_name, common::defs::OperationState final_state)
{
    std::lock_guard<std::mutex> lock(model_stats_mutex_);
    auto& s = model_stats_[model_name];
    if (s.current_inflight > 0)
        --s.current_inflight;
    switch (final_state)
    {
        case common::defs::OperationState::SUCCESS:
            ++s.total_succeeded;
            break;
        case common::defs::OperationState::FAILURE:
            ++s.total_failed;
            break;
        case common::defs::OperationState::CANCELED:
            ++s.total_canceled;
            break;
        default:
            break;
    }
}

}  // namespace IP::scheduler