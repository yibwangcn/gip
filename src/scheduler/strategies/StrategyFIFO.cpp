
#include "scheduler/strategies/StrategyFIFO.hpp"

namespace IP::scheduler
{
    bool StrategyFIFO::admit(std::shared_ptr<common::defs::Task> task, const SchedulingContext &context)
    {
        (void)context;
        if (task == nullptr)
        {
            return false;
        }

        std::lock_guard<std::mutex> lock(mutex_);
        pending_tasks_.push_back(std::move(task));
        return true;
    }

    std::optional<common::defs::DispatchUnit> StrategyFIFO::select(const SelectionContext &context)
    {
        (void)context;
        std::lock_guard<std::mutex> lock(mutex_);
        if (pending_tasks_.empty())
        {
            return std::nullopt;
        }

        common::defs::DispatchUnit unit;
        unit.strategy = kind();
        unit.model_name = pending_tasks_.front()->modelName();
        unit.tasks.push_back(pending_tasks_.front());
        pending_tasks_.pop_front();
        return unit;
    }

    bool StrategyFIFO::revoke(const std::string &request_id)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto iter = pending_tasks_.begin(); iter != pending_tasks_.end(); ++iter)
        {
            if ((*iter) != nullptr && (*iter)->requestId() == request_id)
            {
                if ((*iter)->cancel_requested != nullptr)
                {
                    (*iter)->cancel_requested->store(true);
                }
                pending_tasks_.erase(iter);
                return true;
            }
        }

        return false;
    }

    size_t StrategyFIFO::pendingTaskCount() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return pending_tasks_.size();
    }

    std::chrono::milliseconds StrategyFIFO::nextReadyIn() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return pending_tasks_.empty() ? std::chrono::hours{24} : std::chrono::milliseconds{0};
    }

    void StrategyFIFO::clear()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        pending_tasks_.clear();
    }

} // namespace IP::scheduler