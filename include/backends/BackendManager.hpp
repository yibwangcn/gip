#ifndef __BACKEND_MANAGER_HPP__
#define __BACKEND_MANAGER_HPP__
#include <memory>
#include <string>
#include <unordered_map>
#include "IBackend.hpp"
#include "common/ModelDef.hpp"

namespace IP::backends
{
    class BackendManager
    {
    public:
        BackendManager() = default;
        ~BackendManager() = default;

        bool registerBackend(const common::defs::BackendType &backend_type, std::shared_ptr<IBackend> backend = nullptr);
        std::shared_ptr<IBackend> getBackend(const common::defs::BackendType &backend_type) const;

    private:
        std::unordered_map<common::defs::BackendType, std::shared_ptr<IBackend>> backends_;
    };

} // namespace IP::backends
#endif /* __BACKEND_MANAGER_HPP__ */