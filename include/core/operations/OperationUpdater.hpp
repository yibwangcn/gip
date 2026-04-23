#ifndef __OPERATION_UPDATER_HPP__
#define __OPERATION_UPDATER_HPP__

#include <memory>
#include <optional>
#include <string>

#include "common/ModelDef.hpp"
#include "common/models/InternalModels.hpp"
#include "core/OperationRepository.hpp"

namespace IP::core::operations
{
    class OperationUpdater
    {
    public:
        explicit OperationUpdater(std::shared_ptr<core::OpRepository> op_repository)
            : op_repository_(std::move(op_repository)) {}

        bool update(const std::string &request_id,
                    common::defs::OperationState new_state,
                    const common::defs::ResponseInfo &response,
                    std::optional<std::string> message = std::nullopt) const;

    private:
        std::shared_ptr<core::OpRepository> op_repository_;
    };

} // namespace IP::core::operations

#endif /* __OPERATION_UPDATER_HPP__ */