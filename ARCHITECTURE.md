# Architecture

## Technology stack

| Technology | Role | Why chosen |
|---|---|---|
| **C++20** | Application language | Native performance for per-frame mesh + simulation work; modern features (designated init, `constexpr`, `unique_ptr`) keep the code compact. |
| **RayLib 5.5** (vendored, `third_party/raylib`) | Windowing, OpenGL rendering, 3D camera, math (`raymath`) | Small, single-purpose 3D library with a clean immediate-mode API and first-class Emscripten/WebGL support — the same code compiles to desktop GL and browser WebGL. |
| **raygui** (vendored, `third_party/raygui.h`) | Immediate-mode UI panel | Header-only, integrates directly with RayLib; no separate UI toolkit or event system needed. |
| **OpenMesh 10.0** (vendored, `third_party/OpenMesh`) | Half-edge mesh + Loop subdivision | Robust half-edge topology makes building the *dual* mesh (Goldberg polyhedron) and querying face adjacency straightforward and correct. |
| **Emscripten** | C++ → WebAssembly compiler | Produces the browser build (`.wasm` + `.js` + preloaded assets) from the identical source via RayLib's `PLATFORM=Web` backend. |

## Architecture overview

The app is a single render/update loop over a small set of plain-data structures:

```
main.cpp
  ├─ AppState            owns everything for one frame
  │    ├─ SimControls    all UI-writable state (rules cfg, camera, flags)
  │    ├─ GameWorld      simulation + geometry + render mesh
  │    ├─ SimPanel       stateless renderer of the control panel
  │    ├─ Camera3D       raylib camera
  │    └─ fonts / textures / atlas
  │
  └─ UpdateFrame(AppState&)   input → step → 3D render → HUD → panel
```

**Data flow (one-way, UI ↔ engine boundary):**

```
        writes                         reads
UI  ─────────────▶  SimControls  ◀─────────────  engine (UpdateFrame)
                    (rules, camera,
                     requests)
engine ─writes─▶  GameWorld  ─reads─▶  UI (generation / alive / counts)
                 (derived state)
```

- **`SimControls`** (`sim_controls.h`) is the single source of everything a UI
  can change: the `GameConfig` rule parameters, camera angles, and the
  `restart`/`rebuild` request flags. Both the native raygui panel and (in the
  future) a JS/web panel read and write only this struct — the engine never
  talks to any UI directly.
- **`GameWorld`** (`game_world.*`) owns the geometry (`HexSphere`), the
  simulation (`GameField`), and the pre-built render mesh. Derived state
  (generation, alive count, hex/pentagon counts) lives here and is read by the
  UI, never duplicated into it.
- **`GameField`** (`game_field.*`) is the automaton: a cell array parallel to
  the sphere faces, neighbor-count stepping, and connected-cluster seeding.
- **`HexSphere`** (`hexsphere.*`) builds the Goldberg polyhedron.

## Key design decisions

**Sphere as the dual of a geodesic mesh.**
`hexsphere.cpp` starts from an icosahedron, applies OpenMesh's Loop subdivision
N times to get a geodesic triangle mesh, then constructs its **dual**: each
triangle becomes a dual vertex (its centroid, projected to the sphere), and each
original vertex becomes a face (pentagon at the 12 icosahedron vertices,
hexagon everywhere else). This yields correct Goldberg topology and per-face
adjacency for the simulation.

**Pre-built render mesh, one draw call.**
Instead of issuing per-face draw calls each frame, `GameWorld` builds one `Mesh`
and updates only its per-vertex colors/UVs on each simulation step, drawn with a
single `DrawMesh()`. This keeps large subdivisions (10 k+ cells) interactive.

**One codebase, two platforms via `#ifdef PLATFORM_WEB`.**
`main.cpp` splits only where it must: the desktop path runs a `while
(!WindowShouldClose())` loop; the web path hands `UpdateFrame` to
`emscripten_set_main_loop`. `CMakeLists.txt` gates the WASM linker flags,
asset preloading, and a custom HTML shell behind `if(EMSCRIPTEN)`.

**DPI-correct canvas sizing on the web without patching RayLib.**
RayLib's web backend sizes the canvas in CSS pixels (its `FLAG_WINDOW_HIGHDPI`
is a no-op), which blurs everything on high-DPI displays. The app keeps the
canvas backing store at physical-pixel resolution itself (`SetWindowSize(css *
devicePixelRatio)` each frame, CSS size reset in JS) so rendering stays crisp
and mouse coordinates still line up — all in application code, no vendored
files modified.

**Fonts rasterized at their exact on-screen size.**
Separate `Font` instances are loaded for the HUD, panel controls, and the small
credits line, each at the size it's actually drawn — raylib's point-filtered
atlas blurs if a font is scaled up or down, so nothing is.

**Vendored dependencies.**
RayLib, raygui, and OpenMesh are committed under `third_party/` and built via
`add_subdirectory`, so a clone builds with no package manager. (OpenMesh's build
ordinarily produces shared libs on non-Windows; for the Emscripten build the
CMake script shadows `WIN32=TRUE` around its `add_subdirectory` to force the
static-library branch, since WASM can't link `.so` files.)

## Build tooling

| Tool | Role |
|---|---|
| **CMake ≥ 3.24** + **CMakePresets.json** | Configure/build presets: `clang-debug`, `clang-release`, `wasm-debug`, `wasm-release`. |
| **Ninja** | Build executor for all presets. |
| **Clang / LLVM** | C++ compiler for the native builds. |
| **VS Build Tools (C++)** | Supplies the MSVC runtime + Windows SDK that Clang links against on Windows. |
| **Emscripten / emsdk** | Toolchain for the WASM presets (via `$env{EMSDK}` toolchain file). |
| Helper scripts in `tools/` | `cmake_with_vsdev.cmd` (activates the VS dev env for terminal builds), `setup_kits.ps1` (generates the machine-specific VS Code kit), `build_wasm.cmd`/`.sh`. |

Compile commands:

```powershell
# native
.\tools\cmake_with_vsdev.cmd --preset clang-release
.\tools\cmake_with_vsdev.cmd --build --preset clang-release

# web
.\tools\build_wasm.cmd wasm-release C:\path\to\emsdk   # → dist/wasm/
```

## AI tooling used

Built with **Claude Code** (Anthropic's agentic CLI) driving **Claude
Opus 4.x / Sonnet** models. The AI agent read and edited the source directly,
ran the CMake/Emscripten builds, diagnosed compiler/linker/runtime errors from
tool output, and iterated on fixes. See [RETROSPECTIVE.md](RETROSPECTIVE.md) for
the workflow and a breakdown of what worked.

## Agent workflow

A single conversational agent session with direct access to the file system,
shell (PowerShell / bash), and build tools. The loop was: describe a goal or
report a visual/build problem → the agent inspects the relevant files and (when
needed) the vendored library sources → proposes and applies a focused change →
rebuilds both targets to verify it compiles → reports back. 