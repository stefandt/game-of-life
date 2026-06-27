# C++ Development Environment Setup History

This file tracks the steps used to set up the local C++ development environment for this project.

## 2026-06-27

### Initial workspace

- Opened an empty workspace folder in VS Code.
- Decided to create a C++ project built with CMake.
- Chose Clang/LLVM as the target compiler toolchain.

### Toolchain installation

- Started installing LLVM with:

```powershell
winget install LLVM
```

### Notes

- CMake was already available on the system.
- After LLVM installation finishes, verify that Clang is available from the terminal:

```powershell
clang++ --version
clang --version
```

- If the commands are not found immediately after installation, restart the terminal or VS Code so the updated `PATH` is loaded.

### VS Code project configuration

- Created a minimal C++20 application in `src/main.cpp`.
- Added `CMakeLists.txt` for building the `clang_cmake_app` executable.
- Added `CMakePresets.json` with `clang-debug` and `clang-release` presets.
- Configured VS Code settings to use CMake presets and the CMake Tools configuration provider.
- Added VS Code tasks for configuring, building, and running the application.
- Added a VS Code launch configuration for debugging the application.

Expected VS Code extensions:

- C/C++ (`ms-vscode.cpptools`)
- CMake Tools (`ms-vscode.cmake-tools`)

Useful commands:

```powershell
cmake --preset clang-debug
cmake --build --preset clang-debug
.\build\clang-debug\clang_cmake_app.exe
```

### Generator availability check

- Confirmed `clang`, `clang++`, and `cmake` are available in `PATH`.
- `ninja`, `nmake`, and `msbuild` were not available in the current terminal `PATH`.
- Kept `clang-debug` and `clang-release` presets for the preferred `clang++ + Ninja` workflow.
- Added a `clang-cl-vs2022` preset for Visual Studio 2022 with the `ClangCL` toolset.

For the preferred Ninja workflow, install Ninja if it is not already available:

```powershell
winget install Ninja-build.Ninja
```

Then restart VS Code or the terminal and verify:

```powershell
ninja --version
```

### Ninja installation and first configure attempt

- Installed Ninja with:

```powershell
winget install Ninja-build.Ninja
```

- WinGet reported that the `PATH` environment variable was modified and the shell should be restarted.
- In the current shell, `ninja` was not available yet, but the installed executable was found at:

```text
C:\Users\stepan.datsko.RCOFFICE\AppData\Local\Microsoft\WinGet\Packages\Ninja-build.Ninja_Microsoft.Winget.Source_8wekyb3d8bbwe\ninja.exe
```

- Tried configuring the project with CMake, Clang, and the explicit Ninja path.
- CMake found `clang++`, but the compiler link test failed because MSVC runtime libraries were not available to the linker:

```text
lld-link: error: could not open 'oldnames.lib': no such file or directory
lld-link: error: could not open 'msvcrtd.lib': no such file or directory
```

Conclusion: LLVM and Ninja are installed, but the Windows C++ runtime/link libraries are not available in the current environment. The next setup step is to install or activate Visual Studio Build Tools with the C++ workload, or use a MinGW-based Clang toolchain instead.

### Successful build

- Installed the missing Visual Studio Build Tools C++ components from an elevated Visual Studio Installer process:

```text
Microsoft.VisualStudio.Component.VC.Tools.x86.x64
Microsoft.VisualStudio.Component.Windows10SDK.22621
```

- Confirmed that `cl.exe`, `oldnames.lib`, and `msvcrtd.lib` are now installed under Visual Studio Build Tools.
- Configured, built, and ran the project from a Visual Studio developer environment:

```powershell
cmake --fresh --preset clang-debug
cmake --build --preset clang-debug
.\build\clang-debug\clang_cmake_app.exe
```

- Program output:

```text
Hello from C++20, CMake, and Clang!
```

- Updated VS Code tasks so configure/build commands call `VsDevCmd.bat` before running CMake.
