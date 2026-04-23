#include <iostream>
#include <mutex>
#include <numeric>
#include <shared_mutex>

#include "backends/ONNXRuntimeBackend.hpp"

namespace IP::backends
{
namespace
{
size_t tensorElementCount(const IP::common::defs::TensorShape& shape)
{
    if (shape.empty())
    {
        return 0;
    }

    return std::accumulate(shape.begin(), shape.end(), static_cast<size_t>(1),
        [](size_t acc, int64_t dim)
        {
            return dim > 0 ? acc * static_cast<size_t>(dim) : static_cast<size_t>(0);
        });
}
}  // namespace

ONNXRuntimeBackend::ORTSession::ORTSession(std::shared_ptr<const common::defs::ModelConf> model_conf, Ort::Env& env,
    const Ort::SessionOptions& session_options)
    : model_conf_(std::move(model_conf))
    , session_(std::make_unique<Ort::Session>(env, model_conf_->model_path.c_str(), session_options))
{
    input_names_.reserve(model_conf_->input_names.size());
    for (const auto& input_name : model_conf_->input_names)
    {
        input_names_.push_back(input_name.c_str());
    }

    output_names_.reserve(model_conf_->output_names.size());
    for (const auto& output_name : model_conf_->output_names)
    {
        output_names_.push_back(output_name.c_str());
    }
}

ONNXRuntimeBackend::ONNXRuntimeBackend() : ort_env_(ORT_LOGGING_LEVEL_WARNING, "ONNXRuntimeBackend")
{
}

ONNXRuntimeBackend::~ONNXRuntimeBackend() = default;

bool ONNXRuntimeBackend::load(std::shared_ptr<const common::defs::ModelConf> model_conf)
{
    std::unique_lock<std::shared_mutex> lock(sessions_mutex_);
    if (sessions_.find(model_conf->model_name) != sessions_.end())
    {
        std::cerr << "Model " << model_conf->model_name << " is already loaded." << std::endl;
        return false;
    }

    auto session = std::make_shared<ORTSession>(model_conf, ort_env_, buildSessionOptions(*model_conf));
    sessions_[model_conf->model_name] = std::move(session);
    std::cout << "Model " << model_conf->model_name << " loaded successfully." << std::endl;
    return true;
}

bool ONNXRuntimeBackend::unload(const std::string& model_name)
{
    std::unique_lock<std::shared_mutex> lock(sessions_mutex_);
    return sessions_.erase(model_name) > 0;
}

bool ONNXRuntimeBackend::warmup(const std::string& model_name)
{
    return findSession(model_name) != nullptr;
}

bool ONNXRuntimeBackend::infer(const common::defs::RequestInfo& request_info, common::defs::ResponseInfo& response_info)
{
    auto session = findSession(request_info.model_name);
    response_info.request_id = request_info.request_id;
    response_info.timestamp = request_info.timestamp;

    if (session == nullptr)
    {
        std::cerr << "Model " << request_info.model_name << " is not loaded." << std::endl;
        response_info.error_message = "model session not found";
        return false;
    }

    const auto model_conf = session->modelConf();
    const auto& input_shapes = request_info.input_shapes.empty() ? model_conf->input_shapes : request_info.input_shapes;
    if (session->inputNames().empty() || input_shapes.empty())
    {
        response_info.error_message = "model input metadata is incomplete";
        return false;
    }

    if (input_shapes.size() != 1 || session->inputNames().size() != 1)
    {
        response_info.error_message = "current ORT backend only supports single-input models";
        return false;
    }

    const auto expected_elements = tensorElementCount(input_shapes.front());
    if (expected_elements == 0)
    {
        response_info.error_message = "input shape is invalid";
        return false;
    }

    if (request_info.input_tensor_values.size() != expected_elements)
    {
        response_info.error_message = "input tensor size does not match input shape";
        return false;
    }

    try
    {
        Ort::MemoryInfo memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
        std::vector<Ort::Value> input_tensors;
        input_tensors.reserve(1);
        input_tensors.emplace_back(Ort::Value::CreateTensor<float>(memory_info,
            const_cast<float*>(request_info.input_tensor_values.data()), request_info.input_tensor_values.size(),
            input_shapes.front().data(), input_shapes.front().size()));

        auto output_tensors = (**session).Run(Ort::RunOptions{ nullptr }, session->inputNames().data(),
            input_tensors.data(), input_tensors.size(), session->outputNames().data(), session->outputNames().size());

        response_info.output_shapes.clear();
        response_info.output_tensor_values.clear();
        response_info.output_tensor_values.reserve(request_info.input_tensor_values.size());

        for (auto& output_tensor : output_tensors)
        {
            auto tensor_info = output_tensor.GetTensorTypeAndShapeInfo();
            const auto output_shape = tensor_info.GetShape();
            response_info.output_shapes.push_back(output_shape);

            const auto element_count = tensor_info.GetElementCount();
            const float* output_data = output_tensor.GetTensorData<float>();
            response_info.output_tensor_values.insert(
                response_info.output_tensor_values.end(), output_data, output_data + element_count);
        }
    }
    catch (const Ort::Exception& ex)
    {
        response_info.error_message = ex.what();
        return false;
    }

    return true;
}

Ort::SessionOptions ONNXRuntimeBackend::buildSessionOptions(const common::defs::ModelConf& model_conf) const
{
    Ort::SessionOptions session_options;
    if (model_conf.intra_op_num_threads > 0)
    {
        session_options.SetIntraOpNumThreads(model_conf.intra_op_num_threads);
    }
    if (model_conf.inter_op_num_threads > 0)
    {
        session_options.SetInterOpNumThreads(model_conf.inter_op_num_threads);
    }
    return session_options;
}

std::shared_ptr<ONNXRuntimeBackend::ORTSession> ONNXRuntimeBackend::findSession(const std::string& model_name) const
{
    std::shared_lock<std::shared_mutex> lock(sessions_mutex_);
    const auto it = sessions_.find(model_name);
    return it != sessions_.end() ? it->second : nullptr;
}
}  // namespace IP::backends