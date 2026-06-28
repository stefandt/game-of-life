#pragma once

#include "hexsphere.h"
#include "raylib.h"

#include <vector>

// Per-cell game state + render color.
// color is updated immediately when alive changes — renderer reads it directly.
struct Cell {
    bool  alive = false;
    Color color = {20, 20, 20, 255};  // dark gray = dead
};

// Game field: parallel to HexSphere::faces.
// cells[i] always corresponds to sphere.faces[i].
struct GameField {
    const HexSphere&  sphere;
    std::vector<Cell> cells;

    // Rules: born when live neighbors in [b_lo, b_hi]; survives when in [s_lo, s_hi]
    int rule_b_lo = 2;
    int rule_b_hi = 2;
    int rule_s_lo = 2;
    int rule_s_hi = 3;

    explicit GameField(const HexSphere& mesh);

    // Place `count` cells as a connected cluster starting from start_face.
    // start_face = -1 picks randomly.
    void seed(int count = 10, int rng_seed = 42, int start_face = -1);

    // Advance one Game of Life generation using current rules
    void step();

    // Set a cell state and update its color immediately
    void set(int idx, bool alive);

private:
    void refresh_color(int idx);
};
