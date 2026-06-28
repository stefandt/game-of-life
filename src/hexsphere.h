#pragma once

#include <vector>
#include "raylib.h"

// ─────────────────────────────────────────────────────────────────────────────
// HexSphere — Goldberg polyhedron represented as a dual mesh.
//
// Construction (see hexsphere.cpp):
//   1. An icosahedron is subdivided N times → geodesic TriMesh (OpenMesh).
//   2. Each triangle of the geodesic mesh contributes one VERTEX to the dual.
//      The dual vertex is the centroid of the triangle, projected onto the sphere.
//   3. Each vertex of the geodesic mesh contributes one FACE to the dual.
//      The face is the polygon formed by the dual vertices of all triangles
//      surrounding that geodesic vertex, ordered CCW when viewed from outside.
//   4. A geodesic vertex with 5 surrounding triangles → pentagonal face (12 total).
//      A geodesic vertex with 6 surrounding triangles → hexagonal face (all others).
//
// Mapping summary:
//   primal geodesic vertex  →  dual face  (= one game cell)
//   primal geodesic triangle →  dual vertex (= one corner of a cell polygon)
//
// Total cells = 10 × 4^N + 2   (hexagons = 10×(4^N−1),  pentagons = 12)
// ─────────────────────────────────────────────────────────────────────────────

struct HexFace {
    // Indices into HexSphere::verts, ordered CCW when viewed from outside the sphere.
    // For a hexagon: 6 entries.  For a pentagon: 5 entries.
    // These are the CORNERS of the polygonal cell boundary on the sphere surface.
    //
    // To iterate edges of this cell:
    //   for (int i = 0; i < verts.size(); ++i)
    //       edge = { sphere.verts[verts[i]], sphere.verts[verts[(i+1) % n]] }
    std::vector<int> verts;

    // Indices into HexSphere::faces — the cells that share an edge with this one.
    // Hexagon has 6 neighbors, pentagon has 5.
    // neighbors[i] shares the edge between verts[i] and verts[(i+1) % n].
    //
    // To iterate neighbors for Game of Life:
    //   for (int nb : face.neighbors)  // nb is a face index
    std::vector<int> neighbors;

    // True for the 12 pentagonal faces (original icosahedron vertices).
    // All other faces are hexagons.
    bool pentagon;
};

// ─────────────────────────────────────────────────────────────────────────────
struct HexSphere {
    // 3D positions of all polygon corners on the sphere surface.
    // These are centroids of primal triangles, projected to radius r.
    // Indexed by face.verts[i].
    std::vector<Vector3> verts;

    // One entry per cell (hexagon or pentagon).
    // faces[i] is the i-th cell; GameField::cells[i] holds its game state.
    // Indexed by the primal geodesic vertex order (OpenMesh iteration order).
    std::vector<HexFace> faces;

    // Total cells = 10 × 4^N + 2   (hexagons = 10×(4^N−1),  pentagons = 12)
    static HexSphere create(int subdivisions, float radius = 1.0f);
};
