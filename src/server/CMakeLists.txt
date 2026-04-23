set(SOURCES
    InferService.cpp
    http/InferServiceApiImpl.cpp
)

add_library(${PROJECT_NAME}-server STATIC ${SOURCES})

target_link_libraries(${PROJECT_NAME}-server
    PUBLIC
    ${PROJECT_NAME}-deps
    ${PROJECT_NAME}-common
    ${PROJECT_NAME}-core
    ${PROJECT_NAME}-model_manager
    ${PROJECT_NAME}-scheduler
    pistache_static
)
