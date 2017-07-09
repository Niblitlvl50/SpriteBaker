
cmake_minimum_required(VERSION 3.6)

project(spritebaker)

set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/bin)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR})

set(CMAKE_BUILD_TYPE Debug)
# set(CMAKE_BUILD_TYPE Release)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 \
    -Wall -Wextra -Wno-long-long -pedantic -Wno-unused-parameter -Wno-unused-function -Wno-unused-variable -Wno-format-pedantic")

file(GLOB_RECURSE source_files "src/*.cpp")
add_executable(spritebaker ${source_files})
target_link_libraries(spritebaker)
