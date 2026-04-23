set(SOURCES
    Inference.cpp
    scenario/IScenario.cpp
    operations/OperationUpdater.cpp
    preprocess/InferencePreprocessor.cpp
    scenario/InferenceScenario.cpp
    OperationRepository.cpp
)

add_library(${PROJECT_NAME}-core STATIC ${SOURCES})

target_link_libraries(${PROJECT_NAME}-core
    PUBLIC
    ${PROJECT_NAME}-deps
    ${PROJECT_NAME}-common
)
