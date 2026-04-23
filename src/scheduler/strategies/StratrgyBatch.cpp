#include "scheduler/strategies/StrategyBatch.hpp"

namespace IP::scheduler
{
bool StrategyBatch::admit(std::shared_ptr<common::defs::Task> task, const SchedulingContext& context)
{
    if (task == nullptr)
        return false;

    std::lock_guard<std::mutex> lock(mutex_);
    auto& q = queues_[task->modelName()];
    if (q.tasks.empty() && context.model_meta.conf != nullptr)
    {
        q.max_batch_size = context.model_meta.conf->max_batch_size > 0 ? context.model_meta.conf->max_batch_size : 1;
        q.batch_timeout_ms = context.model_meta.conf->batch_timeout_ms;
    }
    ++q.total_admitted;
    q.tasks.push_back(std::move(task));
    return true;
}

std::optional<common::defs::DispatchUnit> StrategyBatch::select(const SelectionContext& context)
{
    (void)context;
    auto now = std::chrono::steady_clock::now();
    std::lock_guard<std::mutex> lock(mutex_);

    for (auto& [model_name, q] : queues_)
    {
        if (q.tasks.empty())
            continue;

        const bool size_ready = q.tasks.size() >= q.max_batch_size;
        const auto age = std::chrono::duration_cast<std::chrono::milliseconds>(now - q.tasks.front()->enqueued_at);
        const bool timeout_ready = age >= std::chrono::milliseconds{ q.batch_timeout_ms };

        if (!size_ready && !timeout_ready)
            continue;

        common::defs::DispatchUnit unit;
        unit.strategy = kind();
        unit.model_name = model_name;
        while (unit.tasks.size() < q.max_batch_size && !q.tasks.empty())
        {
            auto t = std::move(q.tasks.front());
            q.tasks.pop_front();
            if (t != nullptr)
                unit.tasks.push_back(std::move(t));
        }
        if (!unit.empty())
        {
            q.total_dispatched += unit.tasks.size();
            return unit;
        }
    }
    return std::nullopt;
}

bool StrategyBatch::revoke(const std::string& request_id)
{
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& [model_name, q] : queues_)
    {
        for (auto it = q.tasks.begin(); it != q.tasks.end(); ++it)
        {
            if (*it != nullptr && (*it)->requestId() == request_id)
            {
                q.tasks.erase(it);
                return true;
            }
        }
    }
    return false;
}

size_t StrategyBatch::pendingTaskCount() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    size_t total = 0;
    for (const auto& [model_name, q] : queues_)
        total += q.tasks.size();
    return total;
}

void StrategyBatch::clear()
{
    std::lock_guard<std::mutex> lock(mutex_);
    queues_.clear();
}

std::chrono::milliseconds StrategyBatch::nextReadyIn() const
{
    auto now = std::chrono::steady_clock::now();
    std::lock_guard<std::mutex> lock(mutex_);

    auto min_wait = std::chrono::hours{ 24 };
    for (const auto& [model_name, q] : queues_)
    {
        if (q.tasks.empty())
            continue;

        // Already reached batch size — ready now
        if (q.tasks.size() >= q.max_batch_size)
            return std::chrono::milliseconds{ 0 };

        // Compute remaining time before the oldest task's deadline fires
        const auto age = std::chrono::duration_cast<std::chrono::milliseconds>(now - q.tasks.front()->enqueued_at);
        const auto remaining = std::chrono::milliseconds{ q.batch_timeout_ms } - age;
        if (remaining <= std::chrono::milliseconds{ 0 })
            return std::chrono::milliseconds{ 0 };

        min_wait = std::min(min_wait, remaining);
    }
    return min_wait;
}

std::vector<common::defs::SchedulerRuntimeSnapshot::BatchWindowState> StrategyBatch::batchWindowStates() const
{
    auto now = std::chrono::steady_clock::now();
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<common::defs::SchedulerRuntimeSnapshot::BatchWindowState> result;
    result.reserve(queues_.size());
    for (const auto& [model_name, q] : queues_)
    {
        if (q.tasks.empty())
            continue;
        common::defs::SchedulerRuntimeSnapshot::BatchWindowState ws;
        ws.model_name = model_name;
        ws.pending_count = q.tasks.size();
        ws.max_batch_size = q.max_batch_size;
        ws.batch_timeout_ms = q.batch_timeout_ms;
        ws.oldest_wait_ms = static_cast<uint32_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(now - q.tasks.front()->enqueued_at).count());
        result.push_back(std::move(ws));
    }
    return result;
}
}  // namespace IP::scheduler