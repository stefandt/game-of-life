#pragma once

#include <vector>
#include "raylib.h"

struct HexFace {
    std::vector<int> verts;  // indices into HexSphere::verts, ordered CCW from outside
    bool pentagon;           // true for the 12 pentagonal faces
};

struct HexSphere {
    std::vector<Vector3> verts;  // dual vertices (triangle centroids) on sphere surface
    std::vector<HexFace>  faces; // dual faces: hexagons + 12 pentagons

    // subdivisions: 1 → 42 faces, 2 → 162, 3 → 642, 4 → 2562
    static HexSphere create(int subdivisions, float radius = 1.0f);
};
