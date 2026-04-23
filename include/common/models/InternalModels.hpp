#ifndef __INTERNAL_MODELS_HPP__
#define __INTERNAL_MODELS_HPP__
#include <atomic>
#include <chrono>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "common/ModelDef.hpp"
#include "common/models/ConfigModels.hpp"

namespace IP::common::defs
{
struct ModelMeta
{
    ModelMeta() = default;
    ModelMeta(
        std::string name, std::shared_ptr<const common::defs::ModelConf> config, common::defs::ModelState model_state)
        : model_name(std::move(name)), conf(std::move(config)), state(model_state)
    {
    }

    std::string model_name;
    std::shared_ptr<const common::defs::ModelConf> conf;
    common::defs::ModelState state;
};

struct RequestInfo
{
    std::string model_name;
    std::optional<std::string> model_version;
    std::string request_id;
    std::string timestamp;
    std::vector<TensorShape> input_shapes;
    std::vector<float> input_tensor_values;
    std::optional<std::string> data;
};

struct ResponseInfo
{
    std::string request_id;
    std::string timestamp;
    std::vector<TensorShape> output_shapes;
    std::vector<float> output_tensor_values;
    std::optional<uint64_t> error_code;
    std::optional<std::string> error_message;
    std::optional<std::string> data;
};

struct OperationInfo
{
    std::string id;
    OperationType type;
    OperationState state;
    std::optional<std::string> message;
    ResponseInfo response;
};

struct Task
{
    explicit Task(common::defs::RequestInfo request_info,
        common::defs::ScheduleStrategy scheduling_strategy = common::defs::ScheduleStrategy::FIFO)
        : request(std::move(request_info))
        , strategy(scheduling_strategy)
        , enqueued_at(std::chrono::steady_clock::now())
        , cancel_requested(std::make_shared<std::atomic_bool>(false))
    {
    }

    ~Task() = default;

    std::string requestId() const
    {
        return request.request_id;
    }
    std::string modelName() const
    {
        return request.model_name;
    }

    common::defs::RequestInfo request;
    common::defs::ResponseInfo response;
    common::defs::ScheduleStrategy strategy;
    std::chrono::steady_clock::time_point enqueued_at;
    std::shared_ptr<std::atomic_bool> cancel_requested;
};

struct DispatchUnit
{
    // // DispatchUnit is the execution payload produced by a strategy.
    // // FIFO returns one task, while batching strategies may return many.
    std::string model_name;
    common::defs::ScheduleStrategy strategy = common::defs::ScheduleStrategy::FIFO;
    std::vector<std::shared_ptr<Task>> tasks;

    bool empty() const
    {
        return tasks.empty();
    }
    size_t size() const
    {
        return tasks.size();
    }
};

struct SchedulerRuntimeSnapshot
{
    size_t worker_count = 0;
    size_t inflight_task_count = 0;
    size_t queued_task_count = 0;
    bool stop_requested = false;

    // Per-model counters maintained by Scheduler (event-driven, no polling)
    struct ModelStats
    {
        std::string model_name;
        uint64_t total_admitted = 0;  // tasks ever submitted to scheduler
        uint64_t total_succeeded = 0;
        uint64_t total_failed = 0;
        uint64_t total_canceled = 0;
        size_t current_inflight = 0;  // tasks currently being executed
    };

    // Batch window state reported by StrategyBatch, per model
    struct BatchWindowState
    {
        std::string model_name;
        size_t pending_count = 0;     // tasks waiting in window
        uint32_t oldest_wait_ms = 0;  // age of oldest task in window
        size_t max_batch_size = 1;
        uint32_t batch_timeout_ms = 20;
    };

    std::unordered_map<std::string, ModelStats> model_stats;
    std::unordered_map<std::string, BatchWindowState> batch_windows;
};

}  // namespace IP::common::defs
#endif /* __INTERNAL_MODELS_HPP__ */