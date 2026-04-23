#ifndef __INFERENCE_SCENARIO_HPP__
#define __INFERENCE_SCENARIO_HPP__

#include <memory>
#include <string>
#include <utility>

#include <pistache/http.h>

#include "common/models/InternalModels.hpp"
#include "core/operations/OperationUpdater.hpp"
#include "core/preprocess/InferencePreprocessor.hpp"
#include "core/scenario/IScenario.hpp"
#include "scheduler/Scheduler.hpp"

namespace IP::core::scenario
{
    class InferenceScenario : public IScenario
    {
    public:
        enum class Step
        {
            ValidateRequest,
            Preprocess,
            DispatchInference,
            FinalizeAccepted,
            Done,
            Failed,
        };

        InferenceScenario(std::shared_ptr<scheduler::Scheduler> scheduler,
                          common::defs::RequestInfo request_info,
                          std::shared_ptr<operations::OperationUpdater> operation_updater = nullptr,
                          std::shared_ptr<preprocess::InferencePreprocessor> preprocessor = nullptr);
        ~InferenceScenario() override = default;

    protected:
        std::pair<Pistache::Http::Code, std::string> precheck() override;
        std::pair<Pistache::Http::Code, std::string> process() override;
        std::pair<Pistache::Http::Code, std::string> afterProcess() override;

    private:
        bool validateRequest();
        bool preprocess();
        bool dispatchInference();
        std::string buildAcceptedMessage() const;
        void fail(Pistache::Http::Code code, std::string message);

        std::shared_ptr<scheduler::Scheduler> scheduler_;
        std::shared_ptr<operations::OperationUpdater> operation_updater_;
        std::shared_ptr<preprocess::InferencePreprocessor> preprocessor_;
        common::defs::RequestInfo request_;
        Step current_step_ = Step::ValidateRequest;
        Pistache::Http::Code terminal_code_ = Pistache::Http::Code::Accepted;
        std::string terminal_message_;
        std::string logger_;
    };
} // namespace IP::core::scenario

#endif /* __INFERENCE_SCENARIO_HPP__ */