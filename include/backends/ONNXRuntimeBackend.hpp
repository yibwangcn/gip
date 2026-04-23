#ifndef __ONNX_RUNTIME_BACKEND_HPP__
#define __ONNX_RUNTIME_BACKEND_HPP__

#include <memory>
#include <shared_mutex>
#include <string>
#include <unordered_map>

#include <onnxruntime_cxx_api.h>

#include "IBackend.hpp"

namespace IP::backends
{
    class ONNXRuntimeBackend : public IBackend
    {
    public:
        class ORTSession
        {
        public:
            ORTSession(std::shared_ptr<const common::defs::ModelConf> model_conf,
                       Ort::Env &env,
                       const Ort::SessionOptions &session_options);

            std::shared_ptr<const common::defs::ModelConf> modelConf() const { return model_conf_; }
            const std::vector<const char *> &inputNames() const { return input_names_; }
            const std::vector<const char *> &outputNames() const { return output_names_; }

            Ort::Session &operator*() { return *session_; }
            const Ort::Session &operator*() const { return *session_; }
            Ort::Session *operator->() { return session_.get(); }
            const Ort::Session *operator->() const { return session_.get(); }

            ~ORTSession() = default;

        private:
            std::shared_ptr<const common::defs::ModelConf> model_conf_;
            std::unique_ptr<Ort::Session> session_;
            std::vector<const char *> input_names_;
            std::vector<const char *> output_names_;
        };

        ONNXRuntimeBackend();
        ~ONNXRuntimeBackend() override;

        ONNXRuntimeBackend(const ONNXRuntimeBackend &) = delete;
        ONNXRuntimeBackend &operator=(const ONNXRuntimeBackend &) = delete;
        ONNXRuntimeBackend(ONNXRuntimeBackend &&) = delete;
        ONNXRuntimeBackend &operator=(ONNXRuntimeBackend &&) = delete;

        bool load(std::shared_ptr<const common::defs::ModelConf> model_conf) override;
        bool unload(const std::string &model_name) override;
        bool warmup(const std::string &model_name) override;
        bool infer(const common::defs::RequestInfo &request,
                   common::defs::ResponseInfo &response) override;
        
        bool inferBatch(const std::vector<common::defs::RequestInfo> &requests,
                        std::vector<common::defs::ResponseInfo> &responses);     

    private:
        Ort::SessionOptions buildSessionOptions(const common::defs::ModelConf &model_conf) const;
        std::shared_ptr<ORTSession> findSession(const std::string &model_name) const;

        Ort::Env ort_env_;
        mutable std::shared_mutex sessions_mutex_;
        std::unordered_map<std::string, std::shared_ptr<ORTSession>> sessions_;
    };
} // namespace IP::backends

#endif /* __ONNX_RUNTIME_BACKEND_HPP__ */