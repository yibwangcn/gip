#ifndef __I_BACKEND_HPP__
#define __I_BACKEND_HPP__

#include "common/models/ConfigModels.hpp"
#include "common/models/InternalModels.hpp"

namespace IP::backends
{
    class IBackend
    {
    public:
        virtual ~IBackend() = default;
        virtual bool load(std::shared_ptr<const common::defs::ModelConf> model_conf) = 0;
        virtual bool unload(const std::string &model_name) = 0;
        virtual bool warmup(const std::string &model_name) = 0;
        virtual bool infer(const common::defs::RequestInfo &request,
                           common::defs::ResponseInfo &response) = 0;
    };
} // namespace IP::backends

#endif /* __I_BACKEND_HPP__ */
