#ifndef __SCHEDULER_HPP__
#define __SCHEDULER_HPP__
#include <chrono>
#include <memory>
#include <optional>
#include <unordered_map>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>

#include "common/models/InternalModels.hpp"
#include "backends/BackendManager.hpp"
#include "model_manager/ModelManager.hpp"
#include "core/OperationRepository.hpp"
#include "common/ThreadPool.hpp"
#include "scheduler/PolicySelector.hpp"
#include "scheduler/strategies/IStrategy.hpp"

namespace IP::scheduler
{
// Scheduler orchestrates the full request lifecycle:
// 1. ask policy which strategy should receive a task,
// 2. drive strategies to emit ready dispatch units,
// 3. execute dispatch units on the worker pool.
class Scheduler
{
public:
    Scheduler(std::shared_ptr<backends::BackendManager> backends,
        std::shared_ptr<model_manager::ModelManager> model_manager, std::shared_ptr<core::OpRepository> op_repository,
        std::shared_ptr<IPolicySelector> policy_selector = nullptr);
    ~Scheduler();

    Scheduler(const Scheduler&) = delete;
    Scheduler& operator=(const Scheduler&) = delete;
    Scheduler(Scheduler&&) = delete;
    Scheduler& operator=(Scheduler&&) = delete;

    bool submitTask(common::defs::RequestInfo request);
    bool cancelTask(const std::string& request_id);
    void stop();
    common::defs::SchedulerRuntimeSnapshot runtimeSnapshot() const;

private:
    void init();
    void dispatchLoop();
    bool admit(std::shared_ptr<common::defs::Task> task);
    std::optional<common::defs::DispatchUnit> selectReadyUnit();
    void execute(common::defs::DispatchUnit unit);
    void updateOperation(const std::string& request_id, common::defs::OperationState new_state,
        const common::defs::ResponseInfo& response, std::optional<std::string> message = std::nullopt);
    void trackTask(const std::shared_ptr<common::defs::Task>& task);
    std::shared_ptr<common::defs::Task> trackedTask(const std::string& request_id) const;
    void forgetTask(const std::string& request_id);
    void finalizeCanceledTask(
        const std::shared_ptr<common::defs::Task>& task, std::optional<std::string> message = std::nullopt);
    SchedulingContext buildAdmissionContext(const std::string& model_name) const;
    SelectionContext buildSelectionContext() const;
    std::shared_ptr<IStrategy> strategy(common::defs::ScheduleStrategy kind) const;
    size_t queuedTaskCount() const;
    std::chrono::milliseconds computeMinWaitDuration() const;

    // Per-model stats: updated on every state transition inside execute().
    // Guarded by model_stats_mutex_.
    struct PerModelStats
    {
        uint64_t total_admitted = 0;
        uint64_t total_succeeded = 0;
        uint64_t total_failed = 0;
        uint64_t total_canceled = 0;
        size_t current_inflight = 0;
    };
    void admitModelStat(const std::string& model_name);
    void recordModelResult(const std::string& model_name, common::defs::OperationState final_state);

    mutable std::mutex model_stats_mutex_;
    std::unordered_map<std::string, PerModelStats> model_stats_;

    std::shared_ptr<backends::BackendManager> backends_;
    std::shared_ptr<model_manager::ModelManager> model_manager_;
    std::shared_ptr<core::OpRepository> op_repository_;
    std::shared_ptr<IPolicySelector> policy_selector_;
    std::atomic<bool> stop_requested_;
    std::atomic<size_t> inflight_task_count_;
    std::string logger_;
    std::shared_ptr<common::ThreadPool::ThreadPool> thread_pool_;
    size_t worker_count_;
    mutable std::mutex tracked_tasks_mutex_;
    std::unordered_map<std::string, std::shared_ptr<common::defs::Task>> tracked_tasks_;
    mutable std::mutex strategies_mutex_;
    std::condition_variable strategies_cv_;
    std::thread dispatcher_thread_;
    std::unordered_map<common::defs::ScheduleStrategy, std::shared_ptr<IStrategy>> strategies_;
};

}  // namespace IP::scheduler
#endif /* __SCHEDULER_HPP__ */