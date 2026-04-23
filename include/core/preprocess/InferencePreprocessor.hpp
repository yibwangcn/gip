#ifndef __INFERENCE_PREPROCESSOR_HPP__
#define __INFERENCE_PREPROCESSOR_HPP__

#include <string>

#include "common/models/InternalModels.hpp"

namespace IP::core::preprocess
{
    class InferencePreprocessor
    {
    public:
        bool process(common::defs::RequestInfo &request, std::string &error_message) const;
    };

} // namespace IP::core::preprocess

#endif /* __INFERENCE_PREPROCESSOR_HPP__ */