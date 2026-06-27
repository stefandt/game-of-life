# Live C++ Project

Minimal C++20 project configured for VS Code, CMake, Clang/LLVM, and Ninja on Windows.

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

VS Code does not activate `VsDevCmd.bat`. The CMake Tools extension uses the active **cmake kit** to configure the project. The kit defined in `.vscode/cmake-kits.json` explicitly sets the same `INCLUDE`, `LIB`, and `LIBPATH` variables that `VsDevCmd.bat` would set.

This ensures that:

- CMake Tools configures the project with the correct environment
- The generated `compile_commands.json` reflects the actual include paths
- IntelliSense resolves standard library headers from the same paths used during build

**Selecting the kit in VS Code:**

`Ctrl+Shift+P` → `CMake: Select a Kit` → `Clang + VS 2022 BuildTools x64`

**When to update `cmake-kits.json`:**

The kit file contains hardcoded paths including the MSVC toolset version (e.g. `14.44.35207`). When Visual Studio Build Tools is updated to a new toolset version, these paths must be regenerated. Run the following in PowerShell to capture the current values:

```powershell
$vsdev = "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat"
$output = cmd /c "`"$vsdev`" -arch=x64 -host_arch=x64 >nul 2>&1 && set"
$output | Select-String '^(INCLUDE|LIB|LIBPATH)='
```

Then update `INCLUDE`, `LIB`, and `LIBPATH` in `.vscode/cmake-kits.json`.

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

## Project Files

### `CMakeLists.txt`

Main CMake project file. Defines the executable target, sets C++20, enables warnings, and configures output names.

```cmake
add_executable(clang_cmake_app src/main.cpp)
target_compile_features(clang_cmake_app PRIVATE cxx_std_20)
target_compile_options(clang_cmake_app PRIVATE -Wall -Wextra -Wpedantic)
set_target_properties(clang_cmake_app PROPERTIES OUTPUT_NAME "live" DEBUG_POSTFIX "_d")
```

`DEBUG_POSTFIX "_d"` appends `_d` to the executable name in Debug configuration, producing `live_d.exe`.

### `CMakePresets.json`

Named configure and build configurations. A hidden `base` preset holds shared settings (Ninja generator, clang++ compiler, `compile_commands.json` export). The `clang-debug` and `clang-release` presets inherit from it.

Used by CMake on the command line (`cmake --preset clang-debug`) and by the CMake Tools extension in VS Code.

### `src/main.cpp`

Application entry point. Currently prints a message and exits.

### `tools/cmake_with_vsdev.cmd`

Helper script for terminal builds. Activates the Visual Studio Build Tools environment via `VsDevCmd.bat`, then forwards all arguments to CMake. Required because Clang on Windows needs `INCLUDE` and `LIB` set to find MSVC headers and libraries.

### `.vscode/cmake-kits.json`

Local cmake kit definition for the CMake Tools extension. Sets the same `INCLUDE`, `LIB`, and `LIBPATH` environment variables that `VsDevCmd.bat` sets, so VS Code builds and IntelliSense use the same paths as terminal builds.

Select this kit once after cloning: `Ctrl+Shift+P` → `CMake: Select a Kit`.

### `.vscode/tasks.json`

VS Code task definitions. Six tasks:

- `CMake: configure clang-debug` — runs cmake configure for debug
- `CMake: build clang-debug` — builds debug (default build task, depends on configure)
- `Run debug app` — runs `live_d.exe`
- `CMake: configure clang-release` — runs cmake configure for release
- `CMake: build clang-release` — builds release
- `Run release app` — runs `live.exe`

All build and configure tasks call `tools/cmake_with_vsdev.cmd` to ensure the MSVC environment is active.

### `.vscode/launch.json`

Debug configuration. Launches `live_d.exe` using the Visual Studio debugger (`cppvsdbg`). Automatically runs `CMake: build clang-debug` before starting the debugger.

### `.vscode/settings.json`

Workspace settings for VS Code and the C/C++ extension.

- `cmake.useCMakePresets: always` — uses `CMakePresets.json`
- `cmake.configureOnOpen: true` — configures automatically on open
- `C_Cpp.default.configurationProvider: ms-vscode.cmake-tools` — IntelliSense config from CMake Tools
- `C_Cpp.default.compileCommands` — points IntelliSense to the generated compilation database
- `C_Cpp.default.compilerPath` — tells the C/C++ extension which compiler model to use

### `build/`

Generated by CMake and Ninja. Not committed to version control.

Key generated files:

```
build/clang-debug/live_d.exe
build/clang-release/live.exe
build/clang-debug/compile_commands.json   — used by IntelliSense
build/clang-debug/build.ninja
build/clang-release/build.ninja
```

---

## Tool Responsibilities

| Tool | Reads | Produces |
|---|---|---|
| **VS Code** | `.vscode/` settings, `CMakePresets.json` via CMake Tools | editor, tasks, debug UI |
| **CMake Tools extension** | `cmake-kits.json`, `CMakePresets.json`, `CMakeLists.txt` | configures project, provides IntelliSense data |
| **CMake** | `CMakeLists.txt`, `CMakePresets.json` | `build.ninja`, `compile_commands.json` |
| **Ninja** | `build.ninja` | calls clang++, produces object files and executables |
| **Clang** | `src/main.cpp`, compiler flags from CMake, MSVC headers via `INCLUDE` | `live_d.exe`, `live.exe` |
| **VsDevCmd.bat** | VS Build Tools installation | sets `INCLUDE`, `LIB`, `LIBPATH` for terminal builds |
| **cmake-kits.json** | — | sets `INCLUDE`, `LIB`, `LIBPATH` for VS Code builds |
