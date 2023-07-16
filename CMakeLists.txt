
cmake_minimum_required(VERSION 3.6)

project(spritebaker)

set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/bin)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR})
set(CMAKE_CXX_STANDARD 14)

# set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_BUILD_TYPE Release)

if(MSVC)
    # 4100 "unreferenced formal parameter"
    # 4244 "conversion from '__int64' to 'uint32_t', possible loss of data"
    # 4245 "conversion from 'int' to 'uint32_t', signed/unsigned mismatch"
    # 4267 "conversion from 'size_t' to 'uint32_t', possible loss of data"
    # 4505 "unreferenced local function has been removed"
    add_compile_options("/W4" "/wd4100" "/wd4244" "/wd4245" "/wd4267" "/wd4505" "$<$<CONFIG:RELEASE>:/O2>")
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS=1)
else()
    add_compile_options("-Wall" "-Wextra" "-Wno-unused-parameter" "$<$<CONFIG:RELEASE>:-O3>")
endif()

file(GLOB_RECURSE source_files "src/*.cpp")
add_executable(spritebaker ${source_files})
target_link_libraries(spritebaker)
