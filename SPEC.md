# Specification

## Overview

An interactive [Conway's Game of Life](https://en.wikipedia.org/wiki/Conway%27s_Game_of_Life)
simulation played not on a flat grid but on the surface of a **Goldberg
polyhedron** — a sphere tiled with hexagons and exactly 12 pentagons. Because the
surface is closed, the automaton has no edges or corners: patterns evolve, drift,
and collide continuously around the whole sphere. The number of cells (mesh
resolution), simulation speed, initial population, and the birth/survival rules
are all adjustable at runtime through an on-screen panel, and the 3D view can be
orbited by hand or left to auto-rotate. It ships as a native Windows build and a
browser (WebAssembly) build from the same C++ source.

## Game rules

The simulation is a totalistic cellular automaton on the sphere's faces:

- **Cell** = one face of the sphere (hexagon or pentagon).
- **Neighbors** = faces sharing an edge (6 for hexagons, 5 for pentagons).
- Each generation, every cell counts its live neighbors and updates
  simultaneously:
  - a **live** cell **survives** if its live-neighbor count is within the
    survival range `[S_lo, S_hi]`, otherwise it dies;
  - a **dead** cell is **born** if its live-neighbor count is within the birth
    range `[B_lo, B_hi]`.

Three selectable rule sets (birth / survival neighbor counts):

| Name    | Born      | Survives  |
|---------|-----------|-----------|
| B2/S23  | exactly 2 | 2–3       |
| B3/S23  | exactly 3 | 2–3 (classic Conway) |
| B2/S34  | exactly 2 | 3–4       |

Because pentagons have only 5 neighbors, rule behavior near the 12 pentagons
differs slightly from the hexagonal majority — an intentional property of
playing Life on a sphere rather than a grid.

## Scope

**In scope**

- Goldberg-polyhedron mesh generation at subdivision levels 2–5
  (162 / 642 / 2 562 / 10 242 cells).
- Simultaneous Game-of-Life stepping over face adjacency.
- Connected-cluster seeding of the initial population with adjustable size.
- Real-time 3D rendering with per-cell coloring, cell edges, and optional
  textured cells (hex/pentagon atlas).
- Runtime controls: subdivision, speed, seed size, rule set, pause, restart,
  camera mode, texture toggle.
- Two build targets from one codebase: native Windows and WebAssembly.

**Out of scope**

- Age-based cell death (aging mechanics). Each cell already tracks how many
  generations it has survived (`Cell::age`), but survival/death is currently
  decided purely by neighbor count — killing cells past a maximum age is a
  possible future extension, not part of this version.
- Persistence (saving or loading patterns).
- Editing individual cells by clicking the sphere.
- Mobile / touch-specific UI.

## Functional requirements

1. Generate a valid Goldberg polyhedron (all-hex except 12 pentagons) for any
   supported subdivision level.
2. Advance the simulation at a user-set rate (0.25–8 steps/second), with pause
   and single-seed restart.
3. Apply the selected rule set immediately when changed.
4. Seed the initial population as one connected cluster of a chosen size.
5. Render the sphere at interactive frame rates with a mouse-orbit and an
   auto-rotate camera.
6. Expose all settings through an on-screen panel and keep the panel/label
   text crisp across display DPI scales.
7. Build and run both as a native `.exe` and as a WASM page in a browser.

## Acceptance criteria

- [ ] Native release build runs and displays a rotating hex sphere.
- [ ] WASM build loads and runs the same simulation in a current browser.
- [ ] Changing subdivision rebuilds the mesh with the correct cell count.
- [ ] Changing the rule set visibly changes simulation behavior, and the panel's
      rule explanation matches the rule actually applied.
- [ ] Pause/resume and restart work from both keyboard and panel.
- [ ] Camera orbits by mouse drag and zooms by wheel; auto-rotate can be toggled.
- [ ] Panel clicks map to the correct controls regardless of window size / DPI.
- [ ] The public GitLab Pages link opens the playable game.
