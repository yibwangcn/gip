#ifndef __MODEL_DEF_HPP__
#define __MODEL_DEF_HPP__
#include <cstdint>
#include <mutex>
#include <shared_mutex>
#include <stdexcept>
#include <string>
#include <vector>

namespace IP::common::defs
{
    using ReadLock = std::shared_lock<std::shared_mutex>;
    using WriteLock = std::unique_lock<std::shared_mutex>;
#define READ_LOCK(mutex) ReadLock rlock(mutex)
#define WRITE_LOCK(mutex) WriteLock wlock(mutex)
    using TensorShape = std::vector<int64_t>;

    enum class BackendType
    {
        ONNXRuntime,
        TensorRT,
        OpenVINO,
        Custom,
    };

    enum class ModelState
    {
        REGISTERED,
        LOADED,
        ENABLED,
        FAILED,
    };

    enum class OperationState
    {
        PENDING,
        ONGOING,
        CANCEL_REQUESTED,
        CANCELED,
        SUCCESS,
        FAILURE,
    };

    enum class OperationType
    {
        LOAD_MODEL,
        UNLOAD_MODEL,
        SUBMIT_INFERENCE,
        CANCEL_INFERENCE,
    };

    enum class ScheduleStrategy
    {
        FIFO,
        Priority,
        MaxThroughput,
    };

} // namespace IP
#endif /* __MODEL_DEF_HPP__ */
