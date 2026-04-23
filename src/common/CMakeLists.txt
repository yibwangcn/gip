set(SOURCES
    ConfigLoad.cpp
    Convertors.cpp
    ThreadPool.cpp
)

add_library(${PROJECT_NAME}-common STATIC ${SOURCES})

target_link_libraries(${PROJECT_NAME}-common
    PUBLIC
    ${PROJECT_NAME}-deps
)
