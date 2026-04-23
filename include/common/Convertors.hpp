#ifndef __CONVERTORS_HPP__
#define __CONVERTORS_HPP__

#include <nlohmann/json.hpp>

#include "common/ModelDef.hpp"
#include "common/models/ConfigModels.hpp"
#include "common/models/InternalModels.hpp"
namespace IP::common::defs
{

    BackendType backendTypeFromString(const std::string &str);
    std::string backendTypeToString(const BackendType &backend_type);
    std::string modelStateToString(const ModelState &state);
    std::string operationStateToString(const OperationState &state);
    std::string operationTypeToString(const OperationType &type);

    void from_json(const nlohmann::json &j, ModelConf &conf);
    void to_json(nlohmann::json &j, const ModelConf &conf);

    void from_json(const nlohmann::json &j, RequestInfo &request);
    void to_json(nlohmann::json &j, const ResponseInfo &response);
    void to_json(nlohmann::json &j, const OperationInfo &operation);
} // namespace IP::common::defs
#endif /* __CONVERTORS_HPP__ */