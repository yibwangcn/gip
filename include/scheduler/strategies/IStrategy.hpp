#ifndef __I_STRATRGY_HPP__
#define __I_STRATRGY_HPP__
#include <chrono>
#include <memory>
#include <optional>
#include <string>

#include "common/models/InternalModels.hpp"
#include "scheduler/PolicySelector.hpp"

namespace IP::scheduler
{
// Strategy owns waiting-set organization within one scheduling mode.
// Examples: FIFO ordering, priority ordering, or batch formation.
class IStrategy
{
public:
    virtual ~IStrategy() = default;

    virtual common::defs::ScheduleStrategy kind() const = 0;
    virtual std::string name() const = 0;
    virtual bool admit(std::shared_ptr<common::defs::Task> task, const SchedulingContext& context) = 0;
    virtual std::optional<common::defs::DispatchUnit> select(const SelectionContext& context) = 0;
    virtual bool revoke(const std::string& request_id) = 0;
    virtual size_t pendingTaskCount() const = 0;
    virtual void clear() = 0;

    // Returns how long until this strategy can emit a ready DispatchUnit.
    // 0ms means a unit is ready right now; a large value means the queue is empty.
    // The Scheduler dispatch loop uses this to sleep for exactly the right duration
    // instead of busy-polling.
    virtual std::chrono::milliseconds nextReadyIn() const
    {
        return std::chrono::hours{ 24 };
    }

    // Returns per-model batch window state for runtime-aware policy decisions.
    // Only StrategyBatch overrides this; other strategies return empty.
    virtual std::vector<common::defs::SchedulerRuntimeSnapshot::BatchWindowState> batchWindowStates() const
    {
        return {};
    }
};

}  // namespace IP::scheduler
#endif /* __I_STRATRGY_HPP__ */
