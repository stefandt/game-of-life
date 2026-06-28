cmake_minimum_required(VERSION 3.24)
project(ClangCMakeApp VERSION 0.1.0 LANGUAGES CXX)

# -- RayLib -------------------------------------------------------------------
# FetchContent — для первоначального скачивания (закомментировано после вендоринга)
# include(FetchContent)
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

set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(third_party/raylib)

# -- OpenMesh ------------------------------------------------------------------
# Clang++ on Windows defines _WIN32 but not WIN32 (no underscores).
# OpenMesh guards Windows-specific code with #ifdef WIN32, so add it.
if(WIN32)
    add_compile_definitions(WIN32 _USE_MATH_DEFINES)
endif()

set(OPENMESH_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(OPENMESH_BUILD_APPS   OFF CACHE BOOL "" FORCE)
set(BUILD_APPS            OFF CACHE BOOL "" FORCE)

# FetchContent — для первоначального скачивания (закомментировано после вендоринга)
# include(FetchContent)
# FetchContent_Declare(
#     openmesh
#     URL      "https://www.graphics.rwth-aachen.de/media/openmesh_static/Releases/10.0/OpenMesh-10.0.0.tar.gz"
#     SOURCE_DIR   "${CMAKE_SOURCE_DIR}/third_party/OpenMesh"
#     BINARY_DIR   "${CMAKE_BINARY_DIR}/_deps/openmesh-build"
#     SUBBUILD_DIR "${CMAKE_BINARY_DIR}/_deps/openmesh-subbuild"
# )
# FetchContent_MakeAvailable(openmesh)

add_subdirectory(third_party/OpenMesh)

# -- Main executable ----------------------------------------------------------
add_executable(clang_cmake_app
    src/main.cpp
    src/hexsphere.cpp
    src/game_field.cpp
)

target_compile_features(clang_cmake_app PRIVATE cxx_std_20)
target_compile_options(clang_cmake_app PRIVATE -Wall -Wextra -Wpedantic)
target_include_directories(clang_cmake_app PRIVATE
    src
    "${CMAKE_SOURCE_DIR}/third_party/OpenMesh/src"
)
target_link_libraries(clang_cmake_app PRIVATE raylib OpenMeshCore OpenMeshTools)

set_target_properties(clang_cmake_app PROPERTIES
    OUTPUT_NAME "live"
    DEBUG_POSTFIX "_d"
)
