set(SOURCES
    Scheduler.cpp
    strategies/IStrategy.cpp
    strategies/StrategyFIFO.cpp
)

add_library(${PROJECT_NAME}-scheduler STATIC ${SOURCES})

target_link_libraries(${PROJECT_NAME}-scheduler
    PUBLIC
    ${PROJECT_NAME}-deps
    ${PROJECT_NAME}-common
    ${PROJECT_NAME}-core
    ${PROJECT_NAME}-model_manager
    ${PROJECT_NAME}-backends
)
