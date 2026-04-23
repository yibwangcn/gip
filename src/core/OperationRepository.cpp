#include "core/OperationRepository.hpp"

namespace IP::core
{
    void OpRepository::addOperation(const common::defs::OperationInfo &op_info)
    {
        common::defs::WRITE_LOCK(operations_mutex_);
        operations_[op_info.id] = op_info;
    }

    std::pair<bool, common::defs::OperationInfo> OpRepository::getOperation(const std::string &op_id) const
    {
        common::defs::READ_LOCK(operations_mutex_);
        auto it = operations_.find(op_id);
        if (it != operations_.end())
        {
            return {true, it->second};
        }
        return {false, {}};
    }

    void OpRepository::updateOperation(const std::string &op_id, const common::defs::OperationInfo &op_info)
    {
        common::defs::WRITE_LOCK(operations_mutex_);
        operations_[op_id] = op_info;
    }

    void OpRepository::removeOperation(const std::string &op_id)
    {
        common::defs::WRITE_LOCK(operations_mutex_);
        operations_.erase(op_id);
    }

    std::string OpRepository::covertState(const common::defs::OperationState &state) const
    {
        switch (state)
        {
        case common::defs::OperationState::ONGOING:
            return "ONGOING";
        case common::defs::OperationState::SUCCESS:
            return "SUCCESS";
        case common::defs::OperationState::FAILURE:
            return "FAILURE";
        default:
            return "UNKNOWN";
        }
    }
} // namespace IP::core