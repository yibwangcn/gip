set(SOURCES
    ModelManager.cpp
)

add_library(${PROJECT_NAME}-model_manager STATIC ${SOURCES})

target_link_libraries(${PROJECT_NAME}-model_manager
    PUBLIC
    ${PROJECT_NAME}-deps
    ${PROJECT_NAME}-common
    ${PROJECT_NAME}-backends
)
