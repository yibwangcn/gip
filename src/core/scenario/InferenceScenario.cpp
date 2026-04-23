#include "core/scenario/InferenceScenario.hpp"

#include <nlohmann/json.hpp>

#include "common/Logger.hpp"

namespace IP::core::scenario
{
    InferenceScenario::InferenceScenario(std::shared_ptr<scheduler::Scheduler> scheduler,
                                         common::defs::RequestInfo request_info,
                                         std::shared_ptr<operations::OperationUpdater> operation_updater,
                                         std::shared_ptr<preprocess::InferencePreprocessor> preprocessor)
        : scheduler_(std::move(scheduler)),
          operation_updater_(std::move(operation_updater)),
          preprocessor_(std::move(preprocessor)),
          request_(std::move(request_info)),
          logger_("scenario::InferenceScenario: ")
    {
        if (preprocessor_ == nullptr)
        {
            preprocessor_ = std::make_shared<preprocess::InferencePreprocessor>();
        }
    }

    std::pair<Pistache::Http::Code, std::string> InferenceScenario::precheck()
    {
        if (scheduler_ == nullptr)
        {
            return {Pistache::Http::Code::Internal_Server_Error, "Scheduler is not initialized"};
        }

        if (request_.request_id.empty())
        {
            return {Pistache::Http::Code::Bad_Request, "Missing request_id"};
        }

        if (request_.model_name.empty())
        {
            return {Pistache::Http::Code::Bad_Request, "Missing model_name"};
        }

        return {Pistache::Http::Code::Ok, {}};
    }

    std::pair<Pistache::Http::Code, std::string> InferenceScenario::process()
    {
        while (!stopRequested())
        {
            switch (current_step_)
            {
            case Step::ValidateRequest:
                if (!validateRequest())
                {
                    current_step_ = Step::Failed;
                    break;
                }
                current_step_ = Step::Preprocess;
                break;
            case Step::Preprocess:
                if (!preprocess())
                {
                    current_step_ = Step::Failed;
                    break;
                }
                current_step_ = Step::DispatchInference;
                break;
            case Step::DispatchInference:
                if (!dispatchInference())
                {
                    current_step_ = Step::Failed;
                    break;
                }
                current_step_ = Step::FinalizeAccepted;
                break;
            case Step::FinalizeAccepted:
                terminal_code_ = Pistache::Http::Code::Accepted;
                terminal_message_ = buildAcceptedMessage();
                current_step_ = Step::Done;
                break;
            case Step::Done:
                return {Pistache::Http::Code::Ok, {}};
            case Step::Failed:
                return {terminal_code_, terminal_message_};
            }
        }

        return {Pistache::Http::Code::Service_Unavailable, "Inference scenario stopped"};
    }

    std::pair<Pistache::Http::Code, std::string> InferenceScenario::afterProcess()
    {
        return {terminal_code_, terminal_message_};
    }

    bool InferenceScenario::validateRequest()
    {
        if (request_.input_shapes.empty() && request_.input_tensor_values.empty() && !request_.data.has_value())
        {
            fail(Pistache::Http::Code::Bad_Request, "Inference request has no input payload");
            return false;
        }

        return true;
    }

    bool InferenceScenario::preprocess()
    {
        std::string error_message;
        if (preprocessor_ != nullptr && !preprocessor_->process(request_, error_message))
        {
            fail(Pistache::Http::Code::Bad_Request,
                 error_message.empty() ? "Inference preprocessing failed" : error_message);
            return false;
        }

        return true;
    }

    bool InferenceScenario::dispatchInference()
    {
        if (!scheduler_->submitTask(request_))
        {
            fail(Pistache::Http::Code::Service_Unavailable, "Failed to submit inference request to scheduler");
            return false;
        }

        return true;
    }

    std::string InferenceScenario::buildAcceptedMessage() const
    {
        nlohmann::json body = {
            {"request_id", request_.request_id},
            {"model_name", request_.model_name},
            {"stage", "submitted"},
            {"scenario", "InferenceScenario"},
        };
        return body.dump();
    }

    void InferenceScenario::fail(Pistache::Http::Code code, std::string message)
    {
        TRC_WARNING << logger_ << message;
        terminal_code_ = code;
        terminal_message_ = std::move(message);
        if (operation_updater_ != nullptr)
        {
            common::defs::ResponseInfo response;
            response.request_id = request_.request_id;
            response.error_message = terminal_message_;
            operation_updater_->update(request_.request_id,
                                       common::defs::OperationState::FAILURE,
                                       response,
                                       terminal_message_);
        }
    }
} // namespace IP::core::scenario