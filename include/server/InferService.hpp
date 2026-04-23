#ifndef __INFER_SERVICE_HPP__
#define __INFER_SERVICE_HPP__

#include <memory>
#include <string>

#include <pistache/http.h>

#include "common/models/InternalModels.hpp"
#include "core/OperationRepository.hpp"
#include "model_manager/ModelManager.hpp"
#include "scheduler/Scheduler.hpp"

namespace IP::server
{
    class InferService
    {
    public:
        InferService(
            std::shared_ptr<model_manager::ModelManager> model_manager,
            std::shared_ptr<scheduler::Scheduler> scheduler,
            std::shared_ptr<core::OpRepository> op_repository)
            : model_manager_(std::move(model_manager)),
              scheduler_(std::move(scheduler)),
              op_repository_(std::move(op_repository)) {};
        ~InferService() = default;

        std::pair<Pistache::Http::Code, std::string> registerModel(const std::string &model_name);
        std::pair<Pistache::Http::Code, std::string> unregisterModel(const std::string &model_name);
        std::pair<Pistache::Http::Code, std::string> loadModel(const std::string &model_name);
        std::pair<Pistache::Http::Code, std::string> unloadModel(const std::string &model_name);
        std::pair<Pistache::Http::Code, std::string> enableModel(const std::string &model_name);
        std::pair<Pistache::Http::Code, std::string> disableModel(const std::string &model_name);
        std::pair<Pistache::Http::Code, std::string> getModelState(const std::string &model_name);

        std::pair<Pistache::Http::Code, std::string> submitInferenceRequest(common::defs::RequestInfo request_info);
        std::pair<Pistache::Http::Code, std::string> getOperationStatus(const std::string &operation_id);
        std::pair<Pistache::Http::Code, std::string> getOperationResult(const std::string &operation_id);
        void deleteOperation(const std::string &operation_id);

        std::pair<Pistache::Http::Code, std::string> cancelInference(const std::string &pre_request_id);

    private:
        std::shared_ptr<model_manager::ModelManager> model_manager_;
        std::shared_ptr<scheduler::Scheduler> scheduler_;
        std::shared_ptr<core::OpRepository> op_repository_;
    };
} // namespace IP::server

#endif /* __INFER_SERVICE_HPP__ */