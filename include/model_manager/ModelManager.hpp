#ifndef __MODEL_MANAGER_HPP__
#define __MODEL_MANAGER_HPP__
#include <string>
#include <unordered_map>
#include <memory>
#include "common/models/InternalModels.hpp"
#include "common/ConfigLoad.hpp"
#include "backends/BackendManager.hpp"
#include "backends/IBackend.hpp"

namespace IP::model_manager
{

    class ModelManager
    {
    public:
        ModelManager(const std::string &conf_path, std::shared_ptr<backends::BackendManager> backends);
        ~ModelManager() = default;

        bool registerModel(const std::string &model_name);
        bool unregisterModel(const std::string &model_name);

        bool loadModel(const std::string &model_name);
        bool unloadModel(const std::string &model_name);

        bool enableModel(const std::string &model_name);
        bool disableModel(const std::string &model_name);

        common::defs::ModelMeta resolveModel(const std::string &model_name);

    private:
        std::string conf_path_prefix_;
        std::unique_ptr<common::ConfigLoad> config_loader_;
        std::unordered_map<std::string, common::defs::ModelMeta> model_registry_;
        std::shared_ptr<backends::BackendManager> backends_;
    };

} // namespace IP::model_manager
#endif /* __MODEL_MANAGER_HPP__ */
