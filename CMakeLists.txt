cmake_minimum_required(VERSION 3.24)
project(ClangCMakeApp VERSION 0.1.0 LANGUAGES CXX)

include(FetchContent)

set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG 5.5
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(raylib)

FetchContent_Declare(
    par
    GIT_REPOSITORY https://github.com/prideout/par.git
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(par)

# par_shapes compiled without strict warnings (C library in C++ context)
add_library(par_shapes_impl OBJECT src/par_shapes_impl.cpp)
target_include_directories(par_shapes_impl PRIVATE ${par_SOURCE_DIR})

add_executable(clang_cmake_app
    src/main.cpp
    src/hexsphere.cpp
    $<TARGET_OBJECTS:par_shapes_impl>
)

target_compile_features(clang_cmake_app PRIVATE cxx_std_20)
target_compile_options(clang_cmake_app PRIVATE -Wall -Wextra -Wpedantic)
target_include_directories(clang_cmake_app PRIVATE src ${par_SOURCE_DIR})
target_link_libraries(clang_cmake_app PRIVATE raylib)

set_target_properties(clang_cmake_app PROPERTIES
    OUTPUT_NAME "live"
    DEBUG_POSTFIX "_d"
)
