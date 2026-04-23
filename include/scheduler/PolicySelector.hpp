#ifndef __POLICY_SELECTOR_HPP__
#define __POLICY_SELECTOR_HPP__

#include "common/models/InternalModels.hpp"

namespace IP::scheduler
{
    // AdmissionContext is used when a new request enters the scheduler.
    // It carries model-level static configuration plus current runtime state so
    // the policy layer can choose which scheduling strategy should own the task.
    struct SchedulingContext
    {
        common::defs::ModelMeta model_meta;
        common::defs::SchedulerRuntimeSnapshot runtime_snapshot;
    };

    // SelectionContext is used when a strategy decides whether it can emit a
    // ready-to-run dispatch unit. It is intentionally runtime-focused.
    struct SelectionContext
    {
        std::string model_name;
        common::defs::SchedulerRuntimeSnapshot runtime_snapshot;
    };

    // PolicySelector only decides cross-strategy routing.
    // It does not reorder tasks within a strategy.
    class IPolicySelector
    {
    public:
        virtual ~IPolicySelector() = default;
        virtual common::defs::ScheduleStrategy selectStrategy(const SchedulingContext &context) const = 0;
    };

    // DefaultPolicySelector is a small policy implementation for the current
    // prototype. It prefers configured strategy, but may degrade to FIFO when
    // runtime conditions do not justify a more advanced strategy.
    class DefaultPolicySelector : public IPolicySelector
    {
    public:
        common::defs::ScheduleStrategy selectStrategy(const SchedulingContext &context) const override
        {
            if (context.model_meta.conf == nullptr)
            {
                return common::defs::ScheduleStrategy::FIFO;
            }

            switch (context.model_meta.conf->schedule_strategy)
            {
            case common::defs::ScheduleStrategy::MaxThroughput:
                if (context.model_meta.conf->max_batch_size > 1 &&
                    context.runtime_snapshot.inflight_task_count < context.runtime_snapshot.worker_count)
                {
                    return common::defs::ScheduleStrategy::MaxThroughput;
                }
                return common::defs::ScheduleStrategy::FIFO;
            case common::defs::ScheduleStrategy::Priority:
                return common::defs::ScheduleStrategy::Priority;
            case common::defs::ScheduleStrategy::FIFO:
            default:
                return common::defs::ScheduleStrategy::FIFO;
            }
        }
    };

} // namespace IP::scheduler

#endif /* __POLICY_SELECTOR_HPP__ */