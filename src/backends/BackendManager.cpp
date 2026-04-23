#include <iostream>
#include "backends/BackendManager.hpp"
#include "backends/ONNXRuntimeBackend.hpp"

namespace IP::backends
{
    bool BackendManager::registerBackend(const common::defs::BackendType &backend_type, std::shared_ptr<IBackend> backend)
    {
        if (backends_.find(backend_type) != backends_.end())
        {
            std::cout << "Backend type " << static_cast<int>(backend_type) << " is already registered." << std::endl;
            return true;
        }
        if (backend == nullptr)
        {
            if (backend_type == common::defs::BackendType::ONNXRuntime)
            {
                backend = std::make_shared<ONNXRuntimeBackend>();
            }
            else
            {
                std::cerr << "Unsupported backend type: " << static_cast<int>(backend_type) << std::endl;
                return false;
            }
        }
        backends_[backend_type] = std::move(backend);
        std::cout << "Backend type " << static_cast<int>(backend_type) << " registered successfully." << std::endl;
        return true;
    }

    std::shared_ptr<IBackend> BackendManager::getBackend(const common::defs::BackendType &backend_type) const
    {
        auto it = backends_.find(backend_type);
        if (it != backends_.end())
        {
            return it->second;
        }
        return nullptr;
    }

} // namespace IP::backends