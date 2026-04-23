
cmake_minimum_required(VERSION 3.16)

project(inference_platform LANGUAGES CXX)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(-g -O0)
else()
    add_compile_options(-O3)
endif()

find_package(Threads REQUIRED)

set(ONNXRUNTIME_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/onnxruntime-linux-x64-1.24.4")
set(ONNXRUNTIME_INCLUDE_DIR "${ONNXRUNTIME_ROOT_DIR}/include")
set(ONNXRUNTIME_LIBRARY_DIR "${ONNXRUNTIME_ROOT_DIR}/lib")

include(${CMAKE_CURRENT_SOURCE_DIR}/third_party/cmake/pistache.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/third_party/cmake/json.cmake)

add_subdirectory(src)