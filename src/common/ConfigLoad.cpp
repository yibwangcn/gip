#include <fstream>
#include "common/Convertors.hpp"
#include "common/ConfigLoad.hpp"
namespace IP::common
{
    std::pair<bool, std::shared_ptr<const common::defs::ModelConf>> ConfigLoad::load(const std::string &config_path)
    {
        try
        {
            std::ifstream file(config_path);
            if (!file.is_open())
            {
                throw std::runtime_error("Could not open config file: " + config_path);
            }
            nlohmann::json json_data;
            file >> json_data;
            common::defs::ModelConf model_conf = json_data;
            return {true, std::make_shared<const common::defs::ModelConf>(std::move(model_conf))};
        }
        catch (const std::exception &e)
        {
            // Handle exceptions (e.g., log the error)
            return {false, nullptr};
        }
    }
} // namespace IP::common
