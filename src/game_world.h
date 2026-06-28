#pragma once

#include "hexsphere.h"
#include "game_field.h"
#include "game_config.h"
#include "cell_atlas.h"
#include "raylib.h"

#include <memory>
#include <vector>

struct GameWorld {
    // ── Geometry ─────────────────────────────────────────────────────────────
    std::unique_ptr<HexSphere>          sphere;
    std::vector<Vector3>                face_centers;
    std::vector<Vector3>                face_tan1;
    std::vector<Vector3>                face_tan2;
    std::vector<std::vector<Vector2>>   face_uvs;   // [-1,1] per vertex
    float sphere_r   = 1.0f;
    int   hex_count  = 0;
    int   pent_count = 0;

    // ── Simulation ────────────────────────────────────────────────────────────
    std::unique_ptr<GameField> field;
    int generation  = 0;
    int alive_count = 0;

    // ── Pre-built render mesh (updated only on step / restart) ────────────────
    // Eliminates per-frame vertex rebuild — single DrawMesh() call per frame.
    Mesh     render_mesh          = {};
    Material render_material      = {};
    bool     render_mesh_ready    = false;

    void rebuild(int subdiv);
    void restart(const GameConfig& cfg, const Camera3D& cam);
    void step();
    void apply_rules(const GameConfig& cfg);
    int  front_face(const Camera3D& cam) const;
    int  total_cells() const { return hex_count + pent_count; }

    // Build the Mesh once after rebuild(); update after step()/restart().
    void build_render_mesh(const CellAtlas& atlas);
    void update_render_mesh(const CellAtlas& atlas);
    void unload_render_mesh();

private:
    void recompute_centers();
    void recount_faces();
    void count_alive();
    void fill_render_dynamic(const CellAtlas& atlas);  // writes texcoords + colors
};
