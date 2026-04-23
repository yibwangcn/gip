set(PISTACHE_BUILD_TESTS OFF CACHE BOOL "Disable pistache tests")

if(NOT PISTACHE_ROOT_DIR)
  set(PISTACHE_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/pistache)
  message("PISTACHE_ROOT_DIR: " ${PISTACHE_ROOT_DIR})
endif()

if(EXISTS "${PISTACHE_ROOT_DIR}/CMakeLists.txt")
  add_subdirectory(${PISTACHE_ROOT_DIR})
  set(PISTACHE_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/pistache/include")
else()
  message(WARNING "The pistache module is broken, can't find CMakeLists.txt")
endif()
