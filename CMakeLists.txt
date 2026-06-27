cmake_minimum_required(VERSION 3.24)
project(ClangCMakeApp VERSION 0.1.0 LANGUAGES CXX)

include(FetchContent)

# -- par_shapes: single header, downloaded to third_party/ --------------------
set(THIRD_PARTY_DIR "${CMAKE_SOURCE_DIR}/third_party")
file(MAKE_DIRECTORY "${THIRD_PARTY_DIR}")

if(NOT EXISTS "${THIRD_PARTY_DIR}/par_shapes.h")
    message(STATUS "Downloading par_shapes.h...")
    file(DOWNLOAD
        "https://raw.githubusercontent.com/prideout/par/master/par_shapes.h"
        "${THIRD_PARTY_DIR}/par_shapes.h"
        SHOW_PROGRESS
    )
endif()

# -- RayLib: full library, downloaded to third_party/raylib/ ------------------
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG 5.5
    GIT_SHALLOW TRUE
    SOURCE_DIR  "${CMAKE_SOURCE_DIR}/third_party/raylib"
    BINARY_DIR  "${CMAKE_BINARY_DIR}/_deps/raylib-build"
    SUBBUILD_DIR "${CMAKE_BINARY_DIR}/_deps/raylib-subbuild"
)
FetchContent_MakeAvailable(raylib)

# -- par_shapes object: compiled without strict warnings ----------------------
add_library(par_shapes_impl OBJECT src/par_shapes_impl.cpp)
target_include_directories(par_shapes_impl PRIVATE ${THIRD_PARTY_DIR})

# -- Main executable ----------------------------------------------------------
add_executable(clang_cmake_app
    src/main.cpp
    src/hexsphere.cpp
    $<TARGET_OBJECTS:par_shapes_impl>
)

target_compile_features(clang_cmake_app PRIVATE cxx_std_20)
target_compile_options(clang_cmake_app PRIVATE -Wall -Wextra -Wpedantic)
target_include_directories(clang_cmake_app PRIVATE src ${THIRD_PARTY_DIR})
target_link_libraries(clang_cmake_app PRIVATE raylib)

set_target_properties(clang_cmake_app PROPERTIES
    OUTPUT_NAME "live"
    DEBUG_POSTFIX "_d"
)
