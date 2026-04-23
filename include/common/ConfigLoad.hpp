#ifndef __CONFIG_LOAD_HPP__
#define __CONFIG_LOAD_HPP__
#include <memory>
#include <string>
#include <utility>

#include "common/models/ConfigModels.hpp"

namespace IP::common
{
    class ConfigLoad
    {
    public:
        ConfigLoad() = default;
        ~ConfigLoad() = default;

        std::pair<bool, std::shared_ptr<const common::defs::ModelConf>> load(const std::string &config_path);
    };
} // namespace IP::common
#endif /* __CONFIG_LOAD_HPP__ */
