#include "core/Inference.hpp"


namespace IP
{
    InferencePlatform::InferencePlatform(const std::string& config_path)
    : config_path_(config_path)
    {
        backend_manager_ = std::make_shared<backends::BackendManager>();
        backend_manager_->registerBackend(common::defs::BackendType::ONNXRuntime);
        auto model_conf_path = config_path_ + "/models/";
        model_manager_ = std::make_shared<model_manager::ModelManager>(model_conf_path, backend_manager_);
        auto op_repository = std::make_shared<core::OpRepository>();
        scheduler_ = std::make_shared<scheduler::Scheduler>(backend_manager_, model_manager_, op_repository);
        infer_service_ = std::make_shared<server::InferService>(model_manager_, scheduler_, op_repository);

        router_ = std::make_shared<Pistache::Rest::Router>();
        infer_api_ = std::make_shared<server::http::InferServiceApiImpl>(router_, infer_service_);
        infer_api_->init();
    }
}
