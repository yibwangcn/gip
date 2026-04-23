set(SOURCES
    BackendManager.cpp
    ONNXRuntimeBackend.cpp
)

add_library(${PROJECT_NAME}-backends STATIC ${SOURCES})

target_link_libraries(${PROJECT_NAME}-backends
    PUBLIC
    ${PROJECT_NAME}-deps
    ${PROJECT_NAME}-common
)
