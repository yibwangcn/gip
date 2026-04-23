#include "model_manager/ModelManager.hpp"
#include "common/Convertors.hpp"
namespace IP::model_manager
{
    ModelManager::ModelManager(const std::string &conf_path, std::shared_ptr<backends::BackendManager> backends) : conf_path_prefix_(conf_path), backends_(std::move(backends)), config_loader_(std::make_unique<common::ConfigLoad>())
    {
    }

    bool ModelManager::registerModel(const std::string &model_name)
    {
        if (model_registry_.find(model_name) != model_registry_.end())
        {
            return true;
        }
        auto path = conf_path_prefix_ + model_name + ".json";
        auto [done, def] = config_loader_->load(path);
        if (done)
        {
            common::defs::ModelMeta meta;
            meta.model_name = model_name;
            meta.conf = std::move(def);
            meta.state = common::defs::ModelState::REGISTERED;
            model_registry_[model_name] = std::move(meta);
            return true;
        }
        return false;
    }
    bool ModelManager::unregisterModel(const std::string &model_name)
    {
        if (auto iter = model_registry_.find(model_name); iter != model_registry_.end())
        {
            if (iter->second.state != common::defs::ModelState::REGISTERED)
            {
                model_registry_.erase(model_name);
                return true;
            }
        }
        return false;
    }

    bool ModelManager::loadModel(const std::string &model_name)
    {
        if (backends_ == nullptr)
        {
            return false;
        }
        if (auto iter = model_registry_.find(model_name); iter != model_registry_.end())
        {
            if (iter->second.state == common::defs::ModelState::REGISTERED)
            {
                auto backend = backends_->getBackend(common::defs::backendTypeFromString(iter->second.conf->backend_type));
                if (backend->load(iter->second.conf) && backend->warmup(model_name))
                {
                    iter->second.state = common::defs::ModelState::LOADED;
                    return true;
                }
            }
        }
        return false;
    }

    bool ModelManager::unloadModel(const std::string &model_name)
    {
        if (backends_ == nullptr)
        {
            return false;
        }
        if (auto iter = model_registry_.find(model_name); iter != model_registry_.end())
        {
            if (iter->second.state == common::defs::ModelState::LOADED)
            {
                if (backends_->getBackend(common::defs::backendTypeFromString(iter->second.conf->backend_type))->unload(model_name))
                {
                    iter->second.state = common::defs::ModelState::REGISTERED;
                    return true;
                }
            }
        }
        return false;
    }

    bool ModelManager::enableModel(const std::string &model_name)
    {
        if (auto iter = model_registry_.find(model_name); iter != model_registry_.end())
        {
            if (iter->second.state == common::defs::ModelState::LOADED)
            {
                iter->second.state = common::defs::ModelState::ENABLED;
                return true;
            }
        }
        return false;
    }

    bool ModelManager::disableModel(const std::string &model_name)
    {
        if (auto iter = model_registry_.find(model_name); iter != model_registry_.end())
        {
            if (iter->second.state == common::defs::ModelState::ENABLED)
            {
                iter->second.state = common::defs::ModelState::LOADED;
                return true;
            }
        }
        return false;
    }

    common::defs::ModelMeta ModelManager::resolveModel(const std::string &model_name)
    {
        if (auto iter = model_registry_.find(model_name); iter != model_registry_.end())
        {
            return iter->second;
        }

        common::defs::ModelMeta meta;
        meta.model_name = model_name;
        meta.conf = std::shared_ptr<const common::defs::ModelConf>{};
        meta.state = common::defs::ModelState::FAILED;
        return meta;
    }

} // namespace IP::model_manager
