#ifndef __STRATEGY_BATCH_HPP__
#define __STRATEGY_BATCH_HPP__
#include <chrono>
#include <deque>
#include <mutex>
#include <unordered_map>

#include "common/models/ConfigModels.hpp"
#include "scheduler/strategies/IStrategy.hpp"

namespace IP::scheduler
{
    class StrategyBatch : public IStrategy
    {
    public:
        StrategyBatch() = default;
        ~StrategyBatch() override = default;

        common::defs::ScheduleStrategy kind() const override
        {
            return common::defs::ScheduleStrategy::MaxThroughput;
        }
        std::string name() const override { return "batch"; }

        bool admit(std::shared_ptr<common::defs::Task> task, const SchedulingContext &context) override;
        std::optional<common::defs::DispatchUnit> select(const SelectionContext &context) override;
        bool revoke(const std::string &request_id) override;
        size_t pendingTaskCount() const override;
        void clear() override;
        std::chrono::milliseconds nextReadyIn() const override;
        std::vector<common::defs::SchedulerRuntimeSnapshot::BatchWindowState>
        batchWindowStates() const override;

    private:
        struct ModelQueue
        {
            std::deque<std::shared_ptr<common::defs::Task>> tasks;
            size_t   max_batch_size   = 1;
            uint32_t batch_timeout_ms = 20;
            // rolling stats maintained by admit() / select()
            uint64_t total_admitted   = 0;
            uint64_t total_dispatched = 0;
        };

        mutable std::mutex mutex_;
        std::unordered_map<std::string, ModelQueue> queues_;
    };
} // namespace IP::scheduler
#endif /* __STRATEGY_BATCH_HPP__ */