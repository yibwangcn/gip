
#ifndef __INFERENCE_HPP__
#define __INFERENCE_HPP__
#include <memory>
#include <string>
#include <pistache/router.h>
#include "scheduler/Scheduler.hpp"
#include "model_manager/ModelManager.hpp"
#include "backends/BackendManager.hpp"
#include "server/InferService.hpp"
#include "server/http/InferServiceApiImpl.hpp"

namespace IP
{
    class InferencePlatform
    {
    public:
        explicit InferencePlatform(const std::string &config_path);
        ~InferencePlatform() = default;

    private:
        std::string config_path_;
        std::shared_ptr<Pistache::Rest::Router> router_;
        std::shared_ptr<backends::BackendManager> backend_manager_;
        std::shared_ptr<model_manager::ModelManager> model_manager_;
        std::shared_ptr<scheduler::Scheduler> scheduler_;
        std::shared_ptr<server::InferService> infer_service_;
        std::shared_ptr<server::http::InferServiceApiImpl> infer_api_;
    };
} // namespace IP
#endif /* __INFERENCE_HPP__ */
