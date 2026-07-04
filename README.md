# Live C++ Project

Minimal C++20 project configured for VS Code, CMake, Clang/LLVM, and Ninja on Windows.

The application renders a hexagonal sphere (Goldberg polyhedron) with an interactive 3D camera.

## Required Tools

### Visual Studio Code

IDE and editor. Required extensions:

- C/C++ (`ms-vscode.cpptools`)
- CMake Tools (`ms-vscode.cmake-tools`)

### CMake

Generates native build files from `CMakeLists.txt`.

```powershell
cmake --version
```

### LLVM / Clang

C++ compiler.

```powershell
winget install LLVM
clang++ --version
```

### Ninja

Build system used by the CMake presets.

```powershell
winget install Ninja-build.Ninja
ninja --version
```

### Visual Studio Build Tools — C++ Components

Clang on Windows uses the MSVC runtime and Windows SDK for linking. The following components are required:

- `Microsoft.VisualStudio.Component.VC.Tools.x86.x64`
- `Microsoft.VisualStudio.Component.Windows10SDK.22621`

These provide `cl.exe`, `oldnames.lib`, `msvcrtd.lib`, and the Windows SDK headers and libraries.

---

## How Library and Include Paths Are Resolved

Clang on Windows does not ship its own standard library. It uses the MSVC STL and Windows SDK headers from the Visual Studio Build Tools installation. These paths must be available at compile time and link time.

There are two contexts where paths must be resolved, and each uses a different mechanism.

### Terminal builds

Command-line builds call `tools/cmake_with_vsdev.cmd`, which activates the Visual Studio developer environment before invoking CMake:

```cmd
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64
cmake %*
```

`VsDevCmd.bat` sets the `INCLUDE`, `LIB`, and `LIBPATH` environment variables for the current process. Clang reads `INCLUDE` to find headers and `LIB` to find libraries. Without this step, Clang fails with errors such as:

```
lld-link: error: could not open 'oldnames.lib': no such file or directory
lld-link: error: could not open 'msvcrtd.lib': no such file or directory
```

### VS Code IntelliSense and CMake Tools builds

VS Code does not activate `VsDevCmd.bat`. The CMake Tools extension uses the active **cmake kit** to configure the project. The kit defined in `.vscode/cmake-kits.json` explicitly sets the same `INCLUDE`, `LIB`, and `LIBPATH` variables that `VsDevCmd.bat would set.

This ensures that:

- CMake Tools configures the project with the correct environment
- The generated `compile_commands.json` reflects the actual include paths
- IntelliSense resolves standard library headers from the same paths used during build

**Selecting the kit in VS Code:**

`Ctrl+Shift+P` → `CMake: Select a Kit` → `Clang + VS <year> <edition> x64`

**Generating `cmake-kits.json` (required on each machine):**

`.vscode/cmake-kits.json` is not committed to git — it contains machine-specific paths
that differ between VS versions and machines. Run once after cloning:

```powershell
.\tools\setup_kits.ps1
```

The script finds `VsDevCmd.bat`, captures the `INCLUDE`/`LIB`/`LIBPATH` variables it sets,
and writes a correct kit file for the current machine. Works with VS 2019, 2022, Community,
Professional, and BuildTools. Equivalent manual steps:

```powershell
$vsdev  = (Get-ChildItem "${env:ProgramFiles(x86)}\Microsoft Visual Studio" -Filter "VsDevCmd.bat" -Recurse | Select-Object -First 1).FullName
$output = cmd /c "`"$vsdev`" -arch=x64 -host_arch=x64 >nul 2>&1 && set"
$output | Select-String '^(INCLUDE|LIB|LIBPATH)='
```

Then select the kit in VS Code: `Ctrl+Shift+P` → `CMake: Select a Kit`.

### How third-party headers become visible in VS Code

When you write `#include "raylib.h"`, VS Code resolves it through this chain:

```
CMakeLists.txt
  target_link_libraries(clang_cmake_app PRIVATE raylib)
       │
       │  RayLib declares its src/ as PUBLIC include directory
       ▼
  CMake generates compile_commands.json
  with flag: -I third_party/raylib/src
       │
       ▼
  .vscode/settings.json
  "C_Cpp.default.compileCommands": ".../compile_commands.json"
       │
       ▼
  IntelliSense reads -I flags → finds raylib.h → autocomplete works
```

The key is `PUBLIC` in RayLib's own CMakeLists: any target that links against `raylib` automatically inherits its include directories. No manual path configuration is needed in `.vscode/settings.json`.

If IntelliSense stops finding headers after CMake changes, re-run configure:

```powershell
.\tools\cmake_with_vsdev.cmd --preset clang-debug
```

---

## Build and Run

### From the terminal

Debug build:

```powershell
.\tools\cmake_with_vsdev.cmd --preset clang-debug
.\tools\cmake_with_vsdev.cmd --build --preset clang-debug
.\build\clang-debug\live_d.exe
```

Release build:

```powershell
.\tools\cmake_with_vsdev.cmd --preset clang-release
.\tools\cmake_with_vsdev.cmd --build --preset clang-release
.\build\clang-release\live.exe
```

### From VS Code

| Action | How |
|---|---|
| Build debug (default) | `Ctrl+Shift+B` |
| Build release | `Ctrl+Shift+P` → `Tasks: Run Task` → `CMake: build clang-release` |
| Run debug app | `Ctrl+Shift+P` → `Tasks: Run Task` → `Run debug app` |
| Run release app | `Ctrl+Shift+P` → `Tasks: Run Task` → `Run release app` |
| Debug (F5) | `F5` — builds debug automatically, then launches debugger |

---

## Output Files

```
build/clang-debug/live_d.exe    — debug build
build/clang-release/live.exe    — release build
```

Output names are configured in `CMakeLists.txt` using `OUTPUT_NAME` and `DEBUG_POSTFIX`.

---

## Application: Hexagonal Sphere

The application renders a **Goldberg polyhedron** — a sphere tiled with hexagons and exactly 12 pentagons (the 12 pentagons are a topological requirement; a sphere cannot be tiled with hexagons alone).

### Controls

| Input | Action |
|---|---|
| LMB drag | rotate sphere |
| Mouse wheel | zoom in / out |
| Tab | toggle camera mode |

Camera modes: **Mouse Orbit** (manual control) and **Auto Orbital** (continuous rotation).

### Face counts by subdivision level

| Subdivisions | Hexagons | Pentagons | Total faces |
|---|---|---|---|
| 1 | 30 | 12 | 42 |
| 2 | 150 | 12 | 162 |
| 3 | 630 | 12 | 642 |
| 4 | 2 550 | 12 | 2 562 |

Formula: `10 × (4ᴺ − 1)` hexagons. Default is subdivision level 3.

### How the mesh is constructed

The hexagonal sphere is built as the **dual mesh** of a geodesic sphere:

```
par_shapes_create_subdivided_sphere(N)
    → geodesic sphere: icosahedron subdivided N times,
      all vertices projected onto unit sphere

For each vertex V of the geodesic mesh:
    → collect all adjacent triangles
    → compute centroid of each triangle (projected onto sphere)
    → sort centroids CCW in the tangent plane at V
    → these ordered centroids form one face of the dual mesh

Result: HexSphere
    → 5-sided faces at the 12 original icosahedron vertices (pentagons)
    → 6-sided faces everywhere else (hexagons)
```

### Libraries used

**RayLib** (`third_party/raylib/`)

OpenGL-based rendering library. Provides window creation, 3D camera, and `DrawLine3D` for wireframe rendering. Included in the project as source via `add_subdirectory`.

**par_shapes** (`third_party/raylib/src/external/par_shapes.h`)

Single-header C library bundled inside RayLib. Used for geodesic sphere generation: `par_shapes_create_subdivided_sphere(N)` creates an icosahedron, subdivides it N times, and projects all vertices onto the unit sphere. The implementation is compiled as part of RayLib — no separate compilation needed.

**hexsphere** (`src/hexsphere.h`, `src/hexsphere.cpp`)

Project code. Builds the dual mesh from the par_shapes geodesic output, computing the Goldberg polyhedron topology with correct face winding. The resulting `HexSphere` struct stores vertices and faces, with each face carrying its vertex indices and a pentagon/hexagon flag. This topology is the foundation for the future Game of Life simulation.

---

## Project Files

### `CMakeLists.txt`

Main CMake project file. Links against RayLib via `add_subdirectory`, which automatically propagates RayLib's include directories to the executable target.

### `CMakePresets.json`

Named configure and build configurations. A hidden `base` preset holds shared settings (Ninja generator, clang++ compiler, `compile_commands.json` export). The `clang-debug` and `clang-release` presets inherit from it.

### `src/main.cpp`

Application entry point. Initializes RayLib window, creates the hexagonal sphere, and runs the render loop with mouse-controlled camera.

### `src/hexsphere.h` / `src/hexsphere.cpp`

Hexagonal sphere generator. Calls par_shapes to get a geodesic sphere, then builds the dual mesh (Goldberg polyhedron) by sorting triangle centroids around each vertex in the tangent plane. The `HexSphere` struct holds all vertices and faces with adjacency information.

### `tools/cmake_with_vsdev.cmd`

Helper script for terminal builds. Activates the Visual Studio Build Tools environment via `VsDevCmd.bat`, then forwards all arguments to CMake.

### `third_party/raylib/`

RayLib 5.5 source, vendored directly into the project. Built as part of the CMake project via `add_subdirectory`. Contains `src/external/par_shapes.h` which is used for geodesic sphere generation.

### `.vscode/cmake-kits.json`

Local cmake kit definition. Sets `INCLUDE`, `LIB`, and `LIBPATH` to the same values that `VsDevCmd.bat` sets, ensuring IntelliSense and builds use identical paths.

Select this kit once after cloning: `Ctrl+Shift+P` → `CMake: Select a Kit`.

### `.vscode/tasks.json`

Six build tasks: configure + build + run for both debug and release. All call `tools/cmake_with_vsdev.cmd`.

### `.vscode/launch.json`

Debug configuration. Launches `live_d.exe` using the Visual Studio debugger (`cppvsdbg`), automatically building before launch.

### `.vscode/settings.json`

Workspace settings. Key entries:

- `C_Cpp.default.configurationProvider: ms-vscode.cmake-tools` — IntelliSense config from CMake Tools
- `C_Cpp.default.compileCommands` — points to generated `compile_commands.json`

### `build/`

Generated by CMake and Ninja. Not committed to version control.

```
build/clang-debug/live_d.exe
build/clang-release/live.exe
build/clang-debug/compile_commands.json   — used by IntelliSense
```

---

## Tool Responsibilities

| Tool | Reads | Produces |
|---|---|---|
| **VS Code** | `.vscode/` settings, `CMakePresets.json` via CMake Tools | editor, tasks, debug UI |
| **CMake Tools extension** | `cmake-kits.json`, `CMakePresets.json`, `CMakeLists.txt` | configures project, provides IntelliSense data |
| **CMake** | `CMakeLists.txt`, `CMakePresets.json` | `build.ninja`, `compile_commands.json` |
| **Ninja** | `build.ninja` | calls clang++, produces object files and executables |
| **Clang** | `src/*.cpp`, compiler flags from CMake, MSVC headers via `INCLUDE` | `live_d.exe`, `live.exe` |
| **RayLib** | — | window, OpenGL context, 3D rendering |
| **par_shapes** | — | geodesic sphere mesh (bundled in RayLib) |
| **VsDevCmd.bat** | VS Build Tools installation | sets `INCLUDE`, `LIB`, `LIBPATH` for terminal builds |
| **cmake-kits.json** | — | sets `INCLUDE`, `LIB`, `LIBPATH` for VS Code builds |
