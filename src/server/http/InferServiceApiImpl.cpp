#include <nlohmann/json.hpp>

#include "common/Convertors.hpp"
#include "server/http/InferServiceApiImpl.hpp"

namespace
{
    std::optional<std::string> getBodyField(const Pistache::Rest::Request &request, const std::string &field)
    {
        if (request.body().empty())
        {
            return std::nullopt;
        }

        const auto json = nlohmann::json::parse(request.body(), nullptr, false);
        if (json.is_discarded() || !json.contains(field) || !json.at(field).is_string())
        {
            return std::nullopt;
        }

        return json.at(field).get<std::string>();
    }

    std::optional<std::string> getQueryField(const Pistache::Rest::Request &request, const std::string &field)
    {
        return request.query().get(field);
    }

    std::optional<std::string> getPathParam(const Pistache::Rest::Request &request, const std::string &field)
    {
        try
        {
            return request.param(field).as<std::string>();
        }
        catch (const std::exception &)
        {
            return std::nullopt;
        }
    }
}

namespace IP::server::http
{
    InferServiceApiImpl::InferServiceApiImpl(std::shared_ptr<Pistache::Rest::Router> restRouter,
                                             std::shared_ptr<InferService> inferService)
        : router_(std::move(restRouter)),
          inferService_(std::move(inferService)),
          logger_("InferServiceApiImpl:")
    {
    }

    void InferServiceApiImpl::init()
    {
        setupRoutes();
    }

    void InferServiceApiImpl::setupRoutes()
    {
        using namespace Pistache::Rest;
        const std::string base = "/api/infer/v1";
        router_->addCustomHandler(Routes::bind(&InferServiceApiImpl::default_handler, this));
        Routes::Post(*router_, base + "/model/register", Routes::bind(&InferServiceApiImpl::model_register_handler, this));
        Routes::Post(*router_, base + "/model/unregister", Routes::bind(&InferServiceApiImpl::model_unregister_handler, this));
        Routes::Post(*router_, base + "/model/load", Routes::bind(&InferServiceApiImpl::model_load_handler, this));
        Routes::Get(*router_, base + "/model/load/:operation_id", Routes::bind(&InferServiceApiImpl::model_load_check_handler, this));
        Routes::Delete(*router_, base + "/model/load/:operation_id", Routes::bind(&InferServiceApiImpl::model_load_delete_handler, this));
        Routes::Post(*router_, base + "/model/unload", Routes::bind(&InferServiceApiImpl::model_unload_handler, this));
        Routes::Get(*router_, base + "/model/unload/:operation_id", Routes::bind(&InferServiceApiImpl::model_unload_check_handler, this));
        Routes::Delete(*router_, base + "/model/unload/:operation_id", Routes::bind(&InferServiceApiImpl::model_unload_delete_handler, this));
        Routes::Post(*router_, base + "/model/enable", Routes::bind(&InferServiceApiImpl::model_enable_handler, this));
        Routes::Post(*router_, base + "/model/disable", Routes::bind(&InferServiceApiImpl::model_disable_handler, this));
        Routes::Get(*router_, base + "/model/state", Routes::bind(&InferServiceApiImpl::model_state_handler, this));

        Routes::Get(*router_, base + "/inference/state", Routes::bind(&InferServiceApiImpl::inference_state_handler, this));
        Routes::Post(*router_, base + "/inference/submit", Routes::bind(&InferServiceApiImpl::inference_submit_handler, this));
        Routes::Get(*router_, base + "/inference/submit/:operation_id", Routes::bind(&InferServiceApiImpl::inference_submit_check_handler, this));
        Routes::Get(*router_, base + "/inference/submit/:operation_id/result", Routes::bind(&InferServiceApiImpl::inference_submit_result_handler, this));
        Routes::Delete(*router_, base + "/inference/submit/:operation_id", Routes::bind(&InferServiceApiImpl::inference_submit_delete_handler, this));
        Routes::Get(*router_, base + "/inference/cancel/:operation_id", Routes::bind(&InferServiceApiImpl::inference_cancel_handler, this));
    }

    void InferServiceApiImpl::default_handler(const Pistache::Rest::Request &request, Pistache::Http::ResponseWriter response)
    {
        (void)request;
        response.send(Pistache::Http::Code::Internal_Server_Error, "not supported");
    }

    void InferServiceApiImpl::model_register_handler(const Pistache::Rest::Request &request, Pistache::Http::ResponseWriter response)
    {
        const auto model_name = getBodyField(request, "model_name");
        if (!model_name.has_value() || model_name->empty())
        {
            response.send(Pistache::Http::Code::Bad_Request, "Missing model_name in request body");
            return;
        }

        auto [code, message] = inferService_->registerModel(*model_name);
        response.send(code, message);
    }

    void InferServiceApiImpl::model_unregister_handler(const Pistache::Rest::Request &request, Pistache::Http::ResponseWriter response)
    {
        const auto model_name = getBodyField(request, "model_name");
        if (!model_name.has_value() || model_name->empty())
        {
            response.send(Pistache::Http::Code::Bad_Request, "Missing model_name in request body");
            return;
        }

        auto [code, message] = inferService_->unregisterModel(*model_name);
        response.send(code, message);
    }

    void InferServiceApiImpl::model_load_handler(const Pistache::Rest::Request &request, Pistache::Http::ResponseWriter response)
    {
        const auto model_name = getBodyField(request, "model_name");
        if (!model_name.has_value() || model_name->empty())
        {
            response.send(Pistache::Http::Code::Bad_Request, "Missing model_name in request body");
            return;
        }

        auto [code, message] = inferService_->loadModel(*model_name);
        response.send(code, message);
    }

    void InferServiceApiImpl::model_load_check_handler(const Pistache::Rest::Request &request, Pistache::Http::ResponseWriter response)
    {
        const auto operation_id = getPathParam(request, ":operation_id");
        if (!operation_id.has_value() || operation_id->empty())
        {
            response.send(Pistache::Http::Code::Bad_Request, "Missing or invalid operation_id parameter");
            return;
        }

        auto [code, message] = inferService_->getOperationStatus(*operation_id);
        response.send(code, message);
    }

    void InferServiceApiImpl::model_load_delete_handler(const Pistache::Rest::Request &request, Pistache::Http::ResponseWriter response)
    {
        const auto operation_id = getPathParam(request, ":operation_id");
        if (!operation_id.has_value() || operation_id->empty())
        {
            response.send(Pistache::Http::Code::Bad_Request, "Missing or invalid operation_id parameter");
            return;
        }

        inferService_->deleteOperation(*operation_id);
        response.send(Pistache::Http::Code::No_Content, "");
    }

    void InferServiceApiImpl::model_unload_handler(const Pistache::Rest::Request &request, Pistache::Http::ResponseWriter response)
    {
        const auto model_name = getBodyField(request, "model_name");
        if (!model_name.has_value() || model_name->empty())
        {
            response.send(Pistache::Http::Code::Bad_Request, "Missing model_name in request body");
            return;
        }

        auto [code, message] = inferService_->unloadModel(*model_name);
        response.send(code, message);
    }

    void InferServiceApiImpl::model_unload_check_handler(const Pistache::Rest::Request &request, Pistache::Http::ResponseWriter response)
    {
        const auto operation_id = getPathParam(request, ":operation_id");
        if (!operation_id.has_value() || operation_id->empty())
        {
            response.send(Pistache::Http::Code::Bad_Request, "Missing or invalid operation_id parameter");
            return;
        }

        auto [code, message] = inferService_->getOperationStatus(*operation_id);
        response.send(code, message);
    }

    void InferServiceApiImpl::model_unload_delete_handler(const Pistache::Rest::Request &request, Pistache::Http::ResponseWriter response)
    {
        const auto operation_id = getPathParam(request, ":operation_id");
        if (!operation_id.has_value() || operation_id->empty())
        {
            response.send(Pistache::Http::Code::Bad_Request, "Missing or invalid operation_id parameter");
            return;
        }

        inferService_->deleteOperation(*operation_id);
        response.send(Pistache::Http::Code::No_Content, "");
    }

    void InferServiceApiImpl::model_enable_handler(const Pistache::Rest::Request &request, Pistache::Http::ResponseWriter response)
    {
        const auto model_name = getBodyField(request, "model_name");
        if (!model_name.has_value() || model_name->empty())
        {
            response.send(Pistache::Http::Code::Bad_Request, "Missing model_name in request body");
            return;
        }

        auto [code, message] = inferService_->enableModel(*model_name);
        response.send(code, message);
    }

    void InferServiceApiImpl::model_disable_handler(const Pistache::Rest::Request &request, Pistache::Http::ResponseWriter response)
    {
        const auto model_name = getBodyField(request, "model_name");
        if (!model_name.has_value() || model_name->empty())
        {
            response.send(Pistache::Http::Code::Bad_Request, "Missing model_name in request body");
            return;
        }

        auto [code, message] = inferService_->disableModel(*model_name);
        response.send(code, message);
    }

    void InferServiceApiImpl::model_state_handler(const Pistache::Rest::Request &request, Pistache::Http::ResponseWriter response)
    {
        const auto model_name = getQueryField(request, "model_name");
        if (!model_name.has_value() || model_name->empty())
        {
            response.send(Pistache::Http::Code::Bad_Request, "Missing model_name query parameter");
            return;
        }

        auto [code, message] = inferService_->getModelState(*model_name);
        response.send(code, message);
    }

    void InferServiceApiImpl::inference_state_handler(const Pistache::Rest::Request &request, Pistache::Http::ResponseWriter response)
    {
        const auto operation_id = getQueryField(request, "operation_id");
        if (!operation_id.has_value() || operation_id->empty())
        {
            response.send(Pistache::Http::Code::Bad_Request, "Missing operation_id query parameter");
            return;
        }

        auto [code, message] = inferService_->getOperationStatus(*operation_id);
        response.send(code, message);
    }

    void InferServiceApiImpl::inference_submit_handler(const Pistache::Rest::Request &request, Pistache::Http::ResponseWriter response)
    {
        common::defs::RequestInfo request_info;
        try
        {
            request_info = nlohmann::json::parse(request.body()).get<common::defs::RequestInfo>();
        }
        catch (const nlohmann::json::exception &e)
        {
            response.send(Pistache::Http::Code::Bad_Request, "Invalid JSON format: " + std::string(e.what()));
            return;
        }
        try
        {
            auto [code, message] = inferService_->submitInferenceRequest(request_info);
            response.send(code, message);
        }
        catch (const std::exception &e)
        {
            response.send(Pistache::Http::Code::Internal_Server_Error, "Error submitting inference request: " + std::string(e.what()));
            return;
        }
    }

    void InferServiceApiImpl::inference_submit_check_handler(const Pistache::Rest::Request &request, Pistache::Http::ResponseWriter response)
    {
        std::string operation_id;
        try
        {
            operation_id = request.param(":operation_id").as<std::string>();
        }
        catch (const std::exception &e)
        {
            response.send(Pistache::Http::Code::Bad_Request, "Missing or invalid operation_id parameter");
            return;
        }
        if (operation_id.empty())
        {
            response.send(Pistache::Http::Code::Bad_Request, "Missing request_id parameter");
            return;
        }
        auto [code, message] = inferService_->getOperationStatus(operation_id);
        response.send(code, message);
    }

    void InferServiceApiImpl::inference_submit_result_handler(const Pistache::Rest::Request &request, Pistache::Http::ResponseWriter response)
    {
        std::string operation_id;
        try
        {
            operation_id = request.param(":operation_id").as<std::string>();
        }
        catch (const std::exception &e)
        {
            response.send(Pistache::Http::Code::Bad_Request, "Missing or invalid operation_id parameter");
            return;
        }
        if (operation_id.empty())
        {
            response.send(Pistache::Http::Code::Bad_Request, "Missing request_id parameter");
            return;
        }
        auto [code, message] = inferService_->getOperationResult(operation_id);
        response.send(code, message);
    }

    void InferServiceApiImpl::inference_submit_delete_handler(const Pistache::Rest::Request &request, Pistache::Http::ResponseWriter response)
    {
        std::string operation_id;
        try
        {
            operation_id = request.param(":operation_id").as<std::string>();
        }
        catch (const std::exception &e)
        {
            response.send(Pistache::Http::Code::Bad_Request, "Missing or invalid operation_id parameter");
            return;
        }
        if (operation_id.empty())
        {
            response.send(Pistache::Http::Code::Bad_Request, "Missing request_id parameter");
            return;
        }
        inferService_->deleteOperation(operation_id);
        response.send(Pistache::Http::Code::No_Content, "");
    }

    void InferServiceApiImpl::inference_cancel_handler(const Pistache::Rest::Request &request, Pistache::Http::ResponseWriter response)
    {
        std::string operation_id;
        try
        {
            operation_id = request.param(":operation_id").as<std::string>();
        }
        catch (const std::exception &)
        {
            response.send(Pistache::Http::Code::Bad_Request, "Missing or invalid operation_id parameter");
            return;
        }

        if (operation_id.empty())
        {
            response.send(Pistache::Http::Code::Bad_Request, "Missing request_id parameter");
            return;
        }

        auto [code, message] = inferService_->cancelInference(operation_id);
        response.send(code, message);
    }
}
