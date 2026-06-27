cmake_minimum_required(VERSION 3.24)

project(ClangCMakeApp VERSION 0.1.0 LANGUAGES CXX)

add_executable(clang_cmake_app
    src/main.cpp
)

set_target_properties(clang_cmake_app PROPERTIES
    OUTPUT_NAME "live"
    OUTPUT_NAME_DEBUG "live_d"
)

target_compile_features(clang_cmake_app PRIVATE cxx_std_20)

if(MSVC)
    target_compile_options(clang_cmake_app PRIVATE /W4)
else()
    target_compile_options(clang_cmake_app PRIVATE -Wall -Wextra -Wpedantic)
endif()
