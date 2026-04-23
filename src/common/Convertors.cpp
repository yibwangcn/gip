

#include "common/Convertors.hpp"

namespace IP::common::defs
{

    std::string backendTypeToString(const BackendType &backend_type)
    {
        switch (backend_type)
        {
        case BackendType::ONNXRuntime:
            return "ONNXRuntime";
        case BackendType::TensorRT:
            return "TensorRT";
        case BackendType::OpenVINO:
            return "OpenVINO";
        case BackendType::Custom:
            return "Custom";
        }
        return "Unknown";
    }

    BackendType backendTypeFromString(const std::string &str)
    {
        if (str == "ONNXRuntime")
        {
            return BackendType::ONNXRuntime;
        }
        else if (str == "TensorRT")
        {
            return BackendType::TensorRT;
        }
        else if (str == "OpenVINO")
        {
            return BackendType::OpenVINO;
        }
        else if (str == "Custom")
        {
            return BackendType::Custom;
        }
        else
        {
            throw std::invalid_argument("Invalid backend type: " + str);
        }
    }

    std::string modelStateToString(const ModelState &state)
    {
        switch (state)
        {
        case ModelState::REGISTERED:
            return "REGISTERED";
        case ModelState::LOADED:
            return "LOADED";
        case ModelState::ENABLED:
            return "ENABLED";
        case ModelState::FAILED:
            return "FAILED";
        }
        return "UNKNOWN";
    }

    std::string operationStateToString(const OperationState &state)
    {
        switch (state)
        {
        case OperationState::PENDING:
            return "PENDING";
        case OperationState::ONGOING:
            return "ONGOING";
        case OperationState::CANCEL_REQUESTED:
            return "CANCEL_REQUESTED";
        case OperationState::CANCELED:
            return "CANCELED";
        case OperationState::SUCCESS:
            return "SUCCESS";
        case OperationState::FAILURE:
            return "FAILURE";
        }
        return "UNKNOWN";
    }

    std::string operationTypeToString(const OperationType &type)
    {
        switch (type)
        {
        case OperationType::LOAD_MODEL:
            return "LOAD_MODEL";
        case OperationType::UNLOAD_MODEL:
            return "UNLOAD_MODEL";
        case OperationType::SUBMIT_INFERENCE:
            return "SUBMIT_INFERENCE";
        case OperationType::CANCEL_INFERENCE:
            return "CANCEL_INFERENCE";
        }
        return "UNKNOWN";
    }

    void from_json(const nlohmann::json &j, ModelConf &conf)
    {
        conf.model_name = j.at("model_name").get<std::string>();
        conf.model_version = j.at("model_version").get<std::string>();
        conf.model_path = j.at("model_path").get<std::string>();
        conf.backend_type = j.at("backend_type").get<std::string>();
        conf.engine_path = j.at("engine_path").get<std::string>();
        conf.input_names = j.at("input_names").get<std::vector<std::string>>();
        conf.output_names = j.at("output_names").get<std::vector<std::string>>();
        conf.input_shapes = j.at("input_shapes").get<std::vector<TensorShape>>();
        conf.output_shapes = j.at("output_shapes").get<std::vector<TensorShape>>();
        conf.auto_warmup = j.value("auto_warmup", true);
        conf.max_batch_size = j.value("max_batch_size", static_cast<size_t>(1));
        conf.intra_op_num_threads = j.value("intra_op_num_threads", 0);
        conf.inter_op_num_threads = j.value("inter_op_num_threads", 0);
    }

    void to_json(nlohmann::json &j, const ModelConf &conf)
    {
        j = nlohmann::json::object();
        j["model_name"] = conf.model_name;
        j["model_version"] = conf.model_version;
        j["model_path"] = conf.model_path;
        j["backend_type"] = conf.backend_type;
        j["engine_path"] = conf.engine_path;
        j["input_names"] = conf.input_names;
        j["output_names"] = conf.output_names;
        j["input_shapes"] = conf.input_shapes;
        j["output_shapes"] = conf.output_shapes;
        j["auto_warmup"] = conf.auto_warmup;
        j["max_batch_size"] = conf.max_batch_size;
        j["intra_op_num_threads"] = conf.intra_op_num_threads;
        j["inter_op_num_threads"] = conf.inter_op_num_threads;
    }

    void from_json(const nlohmann::json &j, RequestInfo &request)
    {
        request.model_name = j.at("model_name").get<std::string>();
        request.model_version = j.contains("model_version") ? std::make_optional(j.at("model_version").get<std::string>()) : std::nullopt;
        request.request_id = j.value("request_id", std::string{});
        request.timestamp = j.value("timestamp", std::string{});
        request.input_shapes = j.value("input_shapes", std::vector<TensorShape>{});
        request.input_tensor_values = j.value("input_tensor_values", std::vector<float>{});
        request.data = j.contains("data") ? std::make_optional(j.at("data").get<std::string>()) : std::nullopt;
    }

    void to_json(nlohmann::json &j, const ResponseInfo &response)
    {
        j = nlohmann::json::object();
        j["request_id"] = response.request_id;
        j["timestamp"] = response.timestamp;
        j["output_shapes"] = response.output_shapes;
        j["output_tensor_values"] = response.output_tensor_values;
        if (response.error_code.has_value())
        {
            j["error_code"] = response.error_code.value();
        }
        if (response.error_message.has_value())
        {
            j["error_message"] = response.error_message.value();
        }
        if (response.data.has_value())
        {
            j["data"] = response.data.value();
        }
    }

    void to_json(nlohmann::json &j, const OperationInfo &operation)
    {
        j = nlohmann::json::object();
        j["id"] = operation.id;
        j["type"] = operationTypeToString(operation.type);
        j["state"] = operationStateToString(operation.state);
        if (operation.message.has_value())
        {
            j["message"] = operation.message.value();
        }
        j["response"] = operation.response;
    }

} // namespace IP::common::defs