#pragma once
#include "raylib.h"

// Atlas layout: 2048×256 px, 8 tiles of 256×256 each.
//
// Tile index → cell type:
//   0  Hex  Empty      U: 0.000 → 0.125
//   1  Hex  Life L1    U: 0.125 → 0.250
//   2  Hex  Life L2    U: 0.250 → 0.375
//   3  Hex  Life L3    U: 0.375 → 0.500
//   4  Pent Empty      U: 0.500 → 0.625
//   5  Pent Life L1    U: 0.625 → 0.750
//   6  Pent Life L2    U: 0.750 → 0.875
//   7  Pent Life L3    U: 0.875 → 1.000
//
// Color convention:
//   #000000  transparent/gap  — rendered with alpha 0
//   #000814  cell background
//   #001d3d  empty cell border
//   #003566  alive cell border (all life levels)
//   #FFFFFF  life mask        — multiply by alive_tint() in C++

// Life level: choose which tile to display for alive cells.
enum class LifeLevel : int { L1 = 0, L2 = 1, L3 = 2 };

// UV rectangle within the atlas.
struct AtlasRect {
    float u0, u1;              // horizontal tile range
    float v0 = 0.0f, v1 = 1.0f; // vertical range (always full height)
};

struct CellAtlas {
    Texture2D texture = {};

    // Load atlas from res/atlas_hex_pent_2048x256.png (relative to exe).
    // Returns true on success.
    bool load(const char* path = "res/atlas_hex_pent_2048x256.png");
    void unload();

    // UV rect for a cell based on its shape, state and life level.
    AtlasRect rect(bool pentagon, bool alive,
                   LifeLevel level = LifeLevel::L1) const;

    // Tint colors: alive cells multiply the white (#FFF) mask by this color.
    static constexpr Color ALIVE_TINT = {40, 210, 80,  255};
    static constexpr Color DEAD_TINT  = {255, 255, 255, 255};
};
