#include "server/InferService.hpp"

#include <chrono>

#include <nlohmann/json.hpp>

#include "common/Convertors.hpp"

namespace
{
    std::string generateOperationId()
    {
        const auto now = std::chrono::steady_clock::now().time_since_epoch().count();
        return "op-" + std::to_string(now);
    }
}

namespace IP::server
{
    std::pair<Pistache::Http::Code, std::string> InferService::registerModel(const std::string &model_name)
    {
        if (model_manager_->registerModel(model_name))
        {
            return {Pistache::Http::Code::Ok, "Model registered successfully"};
        }
        else
        {
            return {Pistache::Http::Code::Bad_Request, "Failed to register model"};
        }
    }

    std::pair<Pistache::Http::Code, std::string> InferService::unregisterModel(const std::string &model_name)
    {
        if (model_manager_->unregisterModel(model_name))
        {
            return {Pistache::Http::Code::Ok, "Model unregistered successfully"};
        }
        else
        {
            return {Pistache::Http::Code::Bad_Request, "Failed to unregister model"};
        }
    }

    std::pair<Pistache::Http::Code, std::string> InferService::loadModel(const std::string &model_name)
    {
        if (model_manager_->loadModel(model_name))
        {
            return {Pistache::Http::Code::Ok, "Model loaded successfully"};
        }
        else
        {
            return {Pistache::Http::Code::Bad_Request, "Failed to load model"};
        }
    }

    std::pair<Pistache::Http::Code, std::string> InferService::unloadModel(const std::string &model_name)
    {
        if (model_manager_->unloadModel(model_name))
        {
            return {Pistache::Http::Code::Ok, "Model unloaded successfully"};
        }
        else
        {
            return {Pistache::Http::Code::Bad_Request, "Failed to unload model"};
        }
    }

    std::pair<Pistache::Http::Code, std::string> InferService::enableModel(const std::string &model_name)
    {
        if (model_manager_->enableModel(model_name))
        {
            return {Pistache::Http::Code::Ok, "Model enabled successfully"};
        }
        else
        {
            return {Pistache::Http::Code::Bad_Request, "Failed to enable model"};
        }
    }

    std::pair<Pistache::Http::Code, std::string> InferService::disableModel(const std::string &model_name)
    {
        if (model_manager_->disableModel(model_name))
        {
            return {Pistache::Http::Code::Ok, "Model disabled successfully"};
        }
        else
        {
            return {Pistache::Http::Code::Bad_Request, "Failed to disable model"};
        }
    }

    std::pair<Pistache::Http::Code, std::string> InferService::getModelState(const std::string &model_name)
    {
        const auto meta = model_manager_->resolveModel(model_name);
        if (meta.conf != nullptr)
        {
            nlohmann::json body = {
                {"model_name", meta.model_name},
                {"state", common::defs::modelStateToString(meta.state)},
                {"backend_type", meta.conf->backend_type}};
            return {Pistache::Http::Code::Ok, body.dump()};
        }
        else
        {
            return {Pistache::Http::Code::Not_Found, "Model not found"};
        }
    }

    std::pair<Pistache::Http::Code, std::string> InferService::submitInferenceRequest(common::defs::RequestInfo request_info)
    {
        const auto meta = model_manager_->resolveModel(request_info.model_name);
        if (meta.conf == nullptr)
        {
            return {Pistache::Http::Code::Not_Found, "Model not found"};
        }
        if (meta.state != common::defs::ModelState::ENABLED)
        {
            return {Pistache::Http::Code::Bad_Request, "Model is not enabled"};
        }

        const auto new_operation_id = generateOperationId();
        request_info.request_id = new_operation_id;
        common::defs::OperationInfo op_info{
            .id = new_operation_id,
            .type = common::defs::OperationType::SUBMIT_INFERENCE,
            .state = common::defs::OperationState::PENDING,
            .message = std::nullopt,
            .response = {}};

        op_repository_->addOperation(op_info);
        if (scheduler_->submitTask(request_info))
        {
            return {Pistache::Http::Code::Accepted, nlohmann::json(op_info).dump()};
        }
        else
        {
            op_info.state = common::defs::OperationState::FAILURE;
            op_info.message = "Failed to submit inference request";
            op_repository_->updateOperation(new_operation_id, op_info);
            return {Pistache::Http::Code::Service_Unavailable, "Failed to submit inference request"};
        }
    }

    std::pair<Pistache::Http::Code, std::string> InferService::getOperationStatus(const std::string &operation_id)
    {
        auto [found, op_info] = op_repository_->getOperation(operation_id);
        if (found)
        {
            return {Pistache::Http::Code::Ok, nlohmann::json(op_info).dump()};
        }
        else
        {
            return {Pistache::Http::Code::Not_Found, "Operation not found"};
        }
    }

    std::pair<Pistache::Http::Code, std::string> InferService::getOperationResult(const std::string &operation_id)
    {
        auto [found, op_info] = op_repository_->getOperation(operation_id);
        if (found)
        {
            if (op_info.state == common::defs::OperationState::SUCCESS)
            {
                return {Pistache::Http::Code::Ok, nlohmann::json(op_info.response).dump()};
            }
            else if (op_info.state == common::defs::OperationState::CANCELED)
            {
                return {Pistache::Http::Code::Conflict, "inference was canceled"};
            }
            else if (op_info.state == common::defs::OperationState::CANCEL_REQUESTED)
            {
                return {Pistache::Http::Code::Accepted, "Cancellation is in progress"};
            }
            else if (op_info.state == common::defs::OperationState::FAILURE)
            {
                return {Pistache::Http::Code::Bad_Request, "inference failed, no result available"};
            }
            else
            {
                return {Pistache::Http::Code::Accepted, "Inference is still in progress"};
            }
        }
        else
        {
            return {Pistache::Http::Code::Not_Found, "Operation not found"};
        }
    }

    void InferService::deleteOperation(const std::string &operation_id)
    {
        op_repository_->removeOperation(operation_id);
    }

    std::pair<Pistache::Http::Code, std::string> InferService::cancelInference(const std::string &pre_request_id)
    {
        auto [found, op_info] = op_repository_->getOperation(pre_request_id);
        if (!found)
        {
            return {Pistache::Http::Code::Not_Found, "Operation not found"};
        }

        if (op_info.state == common::defs::OperationState::CANCELED)
        {
            return {Pistache::Http::Code::Ok, nlohmann::json(op_info).dump()};
        }

        if (op_info.state == common::defs::OperationState::SUCCESS ||
            op_info.state == common::defs::OperationState::FAILURE)
        {
            return {Pistache::Http::Code::Conflict, "Operation already finished and cannot be canceled"};
        }

        if (scheduler_->cancelTask(pre_request_id))
        {
            auto [found_after_cancel, canceled_info] = op_repository_->getOperation(pre_request_id);
            if (found_after_cancel)
            {
                if (canceled_info.state == common::defs::OperationState::CANCELED)
                {
                    return {Pistache::Http::Code::Ok, nlohmann::json(canceled_info).dump()};
                }
                if (canceled_info.state == common::defs::OperationState::CANCEL_REQUESTED)
                {
                    return {Pistache::Http::Code::Accepted, nlohmann::json(canceled_info).dump()};
                }
                return {Pistache::Http::Code::Ok, nlohmann::json(canceled_info).dump()};
            }
            return {Pistache::Http::Code::Ok, "Operation canceled"};
        }
        else
        {
            return {Pistache::Http::Code::Conflict, "Operation could not be canceled"};
        }
    }

} // namespace IP::server