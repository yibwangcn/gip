#include "core/preprocess/InferencePreprocessor.hpp"

namespace IP::core::preprocess
{
    bool InferencePreprocessor::process(common::defs::RequestInfo &request, std::string &error_message) const
    {
        if (request.input_shapes.empty() && request.input_tensor_values.empty() && !request.data.has_value())
        {
            error_message = "Inference request has no input payload";
            return false;
        }

        // The first version keeps preprocessing as a lightweight pass-through.
        // Model-specific transformations can grow behind this interface later.
        return true;
    }
} // namespace IP::core::preprocess