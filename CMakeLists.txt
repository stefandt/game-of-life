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

# RayLib: PLATFORM=Web activates Emscripten/GLES2 backend
if(EMSCRIPTEN)
    set(PLATFORM "Web" CACHE STRING "" FORCE)
    # Emscripten does not support shared libraries; force static globally
    # so OpenMesh (which respects BUILD_SHARED_LIBS) builds as .a
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
endif()

set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(third_party/raylib)

# -- OpenMesh ------------------------------------------------------------------
# Clang++ on Windows defines _WIN32 but not WIN32 (no underscores).
# OpenMesh guards Windows-specific code with #ifdef WIN32, so add it.
# Not needed for Emscripten (POSIX-like environment).
if(WIN32 AND NOT EMSCRIPTEN)
    add_compile_definitions(WIN32 _USE_MATH_DEFINES)
endif()

set(OPENMESH_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(OPENMESH_BUILD_APPS   OFF CACHE BOOL "" FORCE)
set(BUILD_APPS            OFF CACHE BOOL "" FORCE)

# On non-Windows (including Emscripten), OpenMesh's cmake unconditionally builds
# SHAREDANDSTATIC and ignores OPENMESH_BUILD_SHARED. Emscripten cannot link .so files.
# Shadow WIN32=TRUE so OpenMesh takes its Windows/static branch during configuration.
if(EMSCRIPTEN)
    set(WIN32 TRUE)
endif()

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
if(EMSCRIPTEN)
    unset(WIN32)
endif()

# -- Main executable ----------------------------------------------------------
add_executable(clang_cmake_app
    src/main.cpp
    src/hexsphere.cpp
    src/game_field.cpp
    src/game_world.cpp
    src/cell_textures.cpp
    src/cell_atlas.cpp
    src/raygui_impl.cpp
    src/sim_panel.cpp
)

# target_compile_features(clang_cmake_app PRIVATE cxx_std_20)
set_target_properties(clang_cmake_app PROPERTIES  CXX_STANDARD 20  CXX_STANDARD_REQUIRED ON)
target_compile_options(clang_cmake_app PRIVATE -Wall -Wextra -Wpedantic)
target_include_directories(clang_cmake_app PRIVATE
    src
    "${CMAKE_SOURCE_DIR}/third_party"
    "${CMAKE_SOURCE_DIR}/third_party/OpenMesh/src"
)
target_link_libraries(clang_cmake_app PRIVATE raylib OpenMeshCore OpenMeshTools)

# macOS requires explicit framework linking for RayLib's OpenGL backend
if(APPLE)
    target_link_libraries(clang_cmake_app PRIVATE
        "-framework OpenGL"
        "-framework Cocoa"
        "-framework IOKit"
        "-framework CoreVideo"
    )
endif()

if(EMSCRIPTEN)
    # Output as .html; Emscripten generates .html + .js + .wasm bundle
    set_target_properties(clang_cmake_app PROPERTIES
        OUTPUT_NAME "live"
        SUFFIX ".html"
    )
    target_link_options(clang_cmake_app PRIVATE
        "SHELL:--preload-file ${CMAKE_SOURCE_DIR}/res@/res"
        "SHELL:--shell-file ${CMAKE_SOURCE_DIR}/res/shell.html"
        -sUSE_GLFW=3
        -sINITIAL_MEMORY=134217728
        -sGL_ENABLE_GET_PROC_ADDRESS=1
        # Expose ccall/cwrap so the HTML/JS panel can call the extern "C"
        # ui_* bridge functions (kept alive via EMSCRIPTEN_KEEPALIVE in main.cpp).
        -sEXPORTED_RUNTIME_METHODS=ccall,cwrap
    )

    # Gather everything needed to host/transfer the app into one folder.
    # live.wasm/live.data are emcc side-outputs next to the .html (not tracked
    # as CMake target files), so they're referenced by path directly.
    set(WASM_DIST_DIR "${CMAKE_SOURCE_DIR}/dist/wasm")
    add_custom_command(TARGET clang_cmake_app POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${WASM_DIST_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy
            "$<TARGET_FILE_DIR:clang_cmake_app>/live.js"
            "$<TARGET_FILE_DIR:clang_cmake_app>/live.wasm"
            "$<TARGET_FILE_DIR:clang_cmake_app>/live.data"
            "${WASM_DIST_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy
            "$<TARGET_FILE_DIR:clang_cmake_app>/live.html"
            "${WASM_DIST_DIR}/index.html"
        COMMENT "Assembling deployable WASM bundle in dist/wasm"
    )
else()
    set_target_properties(clang_cmake_app PROPERTIES
        OUTPUT_NAME "live"
        DEBUG_POSTFIX "_d"
    )
    # Copy res/ folder next to the built executable so LoadTexture can find it
    add_custom_command(TARGET clang_cmake_app POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/res"
            "$<TARGET_FILE_DIR:clang_cmake_app>/res"
        COMMENT "Copying res/ to build output"
    )
endif()
