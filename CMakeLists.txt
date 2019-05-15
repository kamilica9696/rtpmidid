project(rtpmidid)
cmake_minimum_required(VERSION 3.2)
add_definitions("-std=gnu++17 -rdynamic -Wall")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include_directories(${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/src/core)
enable_testing()

add_subdirectory(src)
add_subdirectory(tests)