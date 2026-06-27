cmake_minimum_required(VERSION 3.24)

project(ClangCMakeApp VERSION 0.1.0 LANGUAGES CXX)

add_executable(clang_cmake_app src/main.cpp)

target_compile_features(clang_cmake_app PRIVATE cxx_std_20)
target_compile_options(clang_cmake_app PRIVATE -Wall -Wextra -Wpedantic)

set_target_properties(clang_cmake_app PROPERTIES
    OUTPUT_NAME "live"
    DEBUG_POSTFIX "_d"
)
