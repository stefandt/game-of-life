#pragma once

#include <vector>
#include "raylib.h"

struct HexFace {
    std::vector<int> verts;      // vertex indices into HexSphere::verts, CCW from outside
    std::vector<int> neighbors;  // adjacent face indices (6 for hex, 5 for pent)
    bool pentagon;
};

struct HexSphere {
    std::vector<Vector3> verts;
    std::vector<HexFace>  faces;

    // subdivisions: 1 → 42 faces, 2 → 162, 3 → 642, 4 → 2562
    static HexSphere create(int subdivisions, float radius = 1.0f);
};
