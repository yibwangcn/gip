add_library(${PROJECT_NAME}-deps INTERFACE)

target_include_directories(${PROJECT_NAME}-deps INTERFACE
    ${PROJECT_SOURCE_DIR}/include
    ${PISTACHE_INCLUDE_DIR}
    ${ONNXRUNTIME_INCLUDE_DIR}
    ${PROJECT_SOURCE_DIR}/third_party/json/include
)

target_link_libraries(${PROJECT_NAME}-deps INTERFACE
    Threads::Threads
    nlohmann_json::nlohmann_json
)

add_subdirectory(backends)
add_subdirectory(common)
add_subdirectory(core)
add_subdirectory(model_manager)
add_subdirectory(scheduler)
add_subdirectory(server)

add_executable(${PROJECT_NAME} main.cpp)

target_link_directories(${PROJECT_NAME} PRIVATE
    ${ONNXRUNTIME_LIBRARY_DIR}
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
    ${PROJECT_NAME}-deps
    ${PROJECT_NAME}-common
    ${PROJECT_NAME}-backends
    ${PROJECT_NAME}-core
    ${PROJECT_NAME}-model_manager
    ${PROJECT_NAME}-scheduler
    ${PROJECT_NAME}-server
    pistache_static
    onnxruntime
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    BUILD_RPATH "${ONNXRUNTIME_LIBRARY_DIR}"
    INSTALL_RPATH "${ONNXRUNTIME_LIBRARY_DIR}"
)