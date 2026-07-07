# Build from Source

How to set up the build environment and compile the game — both the native
Windows executable and the WebAssembly bundle.

> To just **play** the game you don't need any of this — see the *Setup* section
> in [README.md](README.md). This file is only for building from source.

---

## 1. Environment setup

### Native toolchain (Windows)

| Tool | Install | Purpose |
|---|---|---|
| LLVM / Clang | `winget install LLVM` | C++20 compiler |
| CMake ≥ 3.24 | `winget install Kitware.CMake` | build generator (`--preset` support) |
| Ninja | `winget install Ninja-build.Ninja` | build executor |
| VS Build Tools (C++) | `winget install Microsoft.VisualStudio.2022.BuildTools` | MSVC runtime + Windows SDK that Clang links against |

After installing Build Tools, open the **Visual Studio Installer** and add the
**"Desktop development with C++"** workload (or at minimum `VC.Tools.x86.x64`
plus a recent **Windows SDK**). Clang on Windows links against the MSVC runtime
and Windows SDK, so these are required even though `cl.exe` is never invoked.

Restart the terminal after installing (winget changes `PATH` but the open shell
doesn't reload it), then verify:

```powershell
clang++ --version
cmake --version
ninja --version
```

Generate the machine-specific VS Code kit once after cloning — it captures the
`INCLUDE`/`LIB`/`LIBPATH` paths Clang needs from your VS installation:

```powershell
.\tools\setup_kits.ps1
```

### WebAssembly toolchain (Emscripten)

Needed only for the WASM build:

```powershell
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
.\emsdk install latest
.\emsdk activate latest
```

`emsdk` requires Python — `winget install Python.Python.3.13` if it's missing.
Verify:

```powershell
emcc --version
```

---

## 2. Build — native (Windows)

Terminal builds go through `tools/cmake_with_vsdev.cmd`, which activates the
Visual Studio developer environment (so Clang finds the MSVC libs) before calling
CMake.

**Release:**

```powershell
.\tools\cmake_with_vsdev.cmd --preset clang-release
.\tools\cmake_with_vsdev.cmd --build --preset clang-release
.\build\clang-release\live.exe
```

**Debug** (produces `live_d.exe`):

```powershell
.\tools\cmake_with_vsdev.cmd --preset clang-debug
.\tools\cmake_with_vsdev.cmd --build --preset clang-debug
.\build\clang-debug\live_d.exe
```

In VS Code: select the kit once (`Ctrl+Shift+P` → *CMake: Select a Kit* →
`Clang + VS <year> <edition> x64`), then build with the CMake Tools presets/tasks.

---

## 3. Build — web (WebAssembly)

Make sure the Emscripten toolchain is installed (section 1), then:

```powershell
.\tools\build_wasm.cmd wasm-release C:\path\to\emsdk
```

The script activates the emsdk environment and runs the `wasm-release` preset.
It produces a deployable bundle in **`dist/wasm/`** (`index.html`, `live.js`,
`live.wasm`, `live.data`) — this is exactly what GitLab Pages serves.

Test it locally (browsers refuse to `fetch` the `.data` file over `file://`, so
serve over HTTP):

```powershell
cd dist\wasm
python -m http.server 8080
# open http://localhost:8080/
```

For the debug WASM build use `wasm-debug` instead of `wasm-release`.

---

## Build presets reference

| Preset | Output |
|---|---|
| `clang-debug` | `build/clang-debug/live_d.exe` |
| `clang-release` | `build/clang-release/live.exe` |
| `wasm-debug` | `build/wasm-debug/live.html` + bundle |
| `wasm-release` | `build/wasm-release/live.html` → copied to `dist/wasm/` |

---

## Troubleshooting

**`cmake: Unknown argument --preset`** — CMake is older than 3.19. VS 2019
sometimes ships CMake 3.18. Install the latest: `winget install Kitware.CMake`.

**`lld-link: could not open 'oldnames.lib' / 'msvcrtd.lib'`** — the MSVC C++
tools aren't installed or the VS dev environment wasn't activated. Add the
"Desktop development with C++" workload, then re-run `.\tools\setup_kits.ps1` and
build via `tools/cmake_with_vsdev.cmd`.

**`could not open 'opengl32.lib'` (x64)** — the Windows SDK shipped with VS 2019
sometimes only has the x86 OpenGL lib. Install a newer SDK
(`winget install Microsoft.WindowsSDK.10.0.22621`) and re-run `setup_kits.ps1`.


**Kit has paths from another machine** — `.vscode/cmake-kits.json` holds absolute,
machine-specific paths and is git-ignored. Always regenerate it with
`.\tools\setup_kits.ps1` on each machine.

**Emscripten: `emcc` not found / `EMSDK` unset** — run `emsdk_env.bat` in the
shell first, or pass the emsdk path to `build_wasm.cmd` as shown above.
