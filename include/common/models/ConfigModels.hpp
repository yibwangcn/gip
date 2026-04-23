#ifndef __MODEL_CONF_HPP__
#define __MODEL_CONF_HPP__
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include "common/ModelDef.hpp"
namespace IP::common::defs
{
    struct ModelConf
    {
        std::string model_name;
        std::string model_version;
        std::string model_path;
        std::string backend_type;
        std::string engine_path;
        std::vector<std::string> input_names;
        std::vector<std::string> output_names;
        std::vector<TensorShape> input_shapes;
        std::vector<TensorShape> output_shapes;
        bool auto_warmup = true;
        size_t max_batch_size = 1;
        uint32_t batch_timeout_ms = 20;
        int intra_op_num_threads = 0;
        int inter_op_num_threads = 0;
        ScheduleStrategy schedule_strategy = ScheduleStrategy::FIFO;
        size_t max_inflight_requests = 1;
        size_t max_queue_size = 128;
        uint64_t max_queue_delay_ms = 0;
        bool allow_request_cancellation = true;
        bool drop_if_queue_full = false;

        friend void from_json(const nlohmann::json &j, ModelConf &conf);
        friend void to_json(nlohmann::json &j, const ModelConf &conf);
    };

} // namespace IP::common::defs
#endif /* __MODEL_CONF_HPP__ */