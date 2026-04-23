#include "core/operations/OperationUpdater.hpp"

namespace IP::core::operations
{
    bool OperationUpdater::update(const std::string &request_id,
                                  common::defs::OperationState new_state,
                                  const common::defs::ResponseInfo &response,
                                  std::optional<std::string> message) const
    {
        if (op_repository_ == nullptr)
        {
            return false;
        }

        auto [found, existing] = op_repository_->getOperation(request_id);
        if (!found)
        {
            return false;
        }

        existing.state = new_state;
        existing.response = response;
        existing.message = std::move(message);
        op_repository_->updateOperation(request_id, existing);
        return true;
    }
} // namespace IP::core::operations