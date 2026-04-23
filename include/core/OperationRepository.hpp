#ifndef __OPERATION_REPOSITORY_HPP__
#define __OPERATION_REPOSITORY_HPP__
#include <mutex>
#include <shared_mutex>
#include <string>
#include <unordered_map>

#include "common/models/InternalModels.hpp"

namespace IP::core
{
    class OpRepository
    {
    public:
        OpRepository() = default;
        ~OpRepository() = default;

        void addOperation(const common::defs::OperationInfo &op_info);
        std::pair<bool, common::defs::OperationInfo> getOperation(const std::string &op_id) const;
        void updateOperation(const std::string &op_id, const common::defs::OperationInfo &op_info);
        void removeOperation(const std::string &op_id);
        std::string covertState(const common::defs::OperationState &state) const;

    private:
        mutable std::shared_mutex operations_mutex_;
        std::unordered_map<std::string, common::defs::OperationInfo> operations_;
    };
}
#endif /* __OPERATION_REPOSITORY_HPP__ */
