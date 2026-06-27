cmake_minimum_required(VERSION 3.24)
project(ClangCMakeApp VERSION 0.1.0 LANGUAGES CXX)

# -- RayLib (contain par_shapes in src/external/) -------------------------
# set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
# FetchContent_Declare(
#     raylib
#     GIT_REPOSITORY https://github.com/raysan5/raylib.git
#     GIT_TAG 5.5
#     GIT_SHALLOW TRUE
#     SOURCE_DIR   "${CMAKE_SOURCE_DIR}/third_party/raylib"
#     BINARY_DIR   "${CMAKE_BINARY_DIR}/_deps/raylib-build"
#     SUBBUILD_DIR "${CMAKE_BINARY_DIR}/_deps/raylib-subbuild"
# )
# FetchContent_MakeAvailable(raylib)


# -- RayLib: локально из third_party/raylib/ -----------------------------------
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(third_party/raylib)

# -- Main executable ----------------------------------------------------------
add_executable(clang_cmake_app
    src/main.cpp
    src/hexsphere.cpp
)

target_compile_features(clang_cmake_app PRIVATE cxx_std_20)
target_compile_options(clang_cmake_app PRIVATE -Wall -Wextra -Wpedantic)
target_include_directories(clang_cmake_app PRIVATE
    src
    "${CMAKE_SOURCE_DIR}/third_party/raylib/src/external"   # par_shapes.h
)
target_link_libraries(clang_cmake_app PRIVATE raylib)

set_target_properties(clang_cmake_app PROPERTIES
    OUTPUT_NAME "live"
    DEBUG_POSTFIX "_d"
)
