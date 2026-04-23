#ifndef __STRATEGY_FIFO_HPP__
#define __STRATEGY_FIFO_HPP__
#include <chrono>
#include <deque>
#include <mutex>

#include "IStrategy.hpp"

namespace IP::scheduler
{
    class StrategyFIFO : public IStrategy
    {
    public:
        StrategyFIFO() = default;
        ~StrategyFIFO() override = default;

        common::defs::ScheduleStrategy kind() const override { return common::defs::ScheduleStrategy::FIFO; }
        std::string name() const override { return "fifo"; }
        bool admit(std::shared_ptr<common::defs::Task> task, const SchedulingContext &context) override;
        std::optional<common::defs::DispatchUnit> select(const SelectionContext &context) override;
        bool revoke(const std::string &request_id) override;
        size_t pendingTaskCount() const override;
        void clear() override;
        std::chrono::milliseconds nextReadyIn() const override;

    private:
        mutable std::mutex mutex_;
        std::deque<std::shared_ptr<common::defs::Task>> pending_tasks_;
    };

} // namespace IP::scheduler
#endif /* __STRATEGY_FIFO_HPP__ */