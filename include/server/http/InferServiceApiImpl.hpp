
#ifndef __INFERENCE_API_IMPL_HPP__
#define __INFERENCE_API_IMPL_HPP__

#include <pistache/endpoint.h>
#include <pistache/http.h>
#include <pistache/router.h>
#include <memory>
#include <vector>
#include "common/Logger.hpp"
#include "server/InferService.hpp"

namespace IP::server::http
{

    class InferServiceApiImpl
    {
    public:
        InferServiceApiImpl(std::shared_ptr<Pistache::Rest::Router> restRouter,
                            std::shared_ptr<InferService> inferService);

        ~InferServiceApiImpl()
        {
        }
        void init();
        void default_handler(const Pistache::Rest::Request &request, Pistache::Http::ResponseWriter response);

        void model_register_handler(const Pistache::Rest::Request &request, Pistache::Http::ResponseWriter response);
        void model_unregister_handler(const Pistache::Rest::Request &request, Pistache::Http::ResponseWriter response);
        void model_load_handler(const Pistache::Rest::Request &request, Pistache::Http::ResponseWriter response);
        void model_load_check_handler(const Pistache::Rest::Request &request, Pistache::Http::ResponseWriter response);
        void model_load_delete_handler(const Pistache::Rest::Request &request, Pistache::Http::ResponseWriter response);
        void model_unload_handler(const Pistache::Rest::Request &request, Pistache::Http::ResponseWriter response);
        void model_unload_check_handler(const Pistache::Rest::Request &request, Pistache::Http::ResponseWriter response);
        void model_unload_delete_handler(const Pistache::Rest::Request &request, Pistache::Http::ResponseWriter response);
        void model_enable_handler(const Pistache::Rest::Request &request, Pistache::Http::ResponseWriter response);
        void model_disable_handler(const Pistache::Rest::Request &request, Pistache::Http::ResponseWriter response);
        void model_state_handler(const Pistache::Rest::Request &request, Pistache::Http::ResponseWriter response);

        void inference_state_handler(const Pistache::Rest::Request &request, Pistache::Http::ResponseWriter response);
        void inference_submit_handler(const Pistache::Rest::Request &request, Pistache::Http::ResponseWriter response);
        void inference_submit_check_handler(const Pistache::Rest::Request &request, Pistache::Http::ResponseWriter response);
        void inference_submit_result_handler(const Pistache::Rest::Request &request, Pistache::Http::ResponseWriter response);
        void inference_submit_delete_handler(const Pistache::Rest::Request &request, Pistache::Http::ResponseWriter response);
        void inference_cancel_handler(const Pistache::Rest::Request &request, Pistache::Http::ResponseWriter response);

    private:
        void setupRoutes();

        std::shared_ptr<Pistache::Rest::Router> router_;
        std::shared_ptr<InferService> inferService_;
        std::string logger_;
    };

} // namespace api

#endif // EMSERVICE_AGGREGATION_API_IMPL_H_