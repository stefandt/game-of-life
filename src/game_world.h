#pragma once

#include "hexsphere.h"
#include "game_field.h"
#include "game_config.h"
#include "raylib.h"

#include <memory>
#include <vector>

// GameWorld owns the geometry and simulation state.
// It bridges the HexSphere topology with the GameField logic.
struct GameWorld {
    // ── Geometry (rebuilt when subdivision changes) ───────────────────────
    std::unique_ptr<HexSphere> sphere;
    std::vector<Vector3>       face_centers;  // centroids projected onto sphere
    float                      sphere_r   = 1.0f;
    int                        hex_count  = 0;
    int                        pent_count = 0;

    // ── Simulation state ──────────────────────────────────────────────────
    std::unique_ptr<GameField> field;
    int generation  = 0;
    int alive_count = 0;

    // Rebuild geometry and field for the given subdivision level
    void rebuild(int subdiv);

    // Reseed the field using config, placing the initial cluster near the camera
    void restart(const GameConfig& cfg, const Camera3D& cam);

    // Advance one simulation step
    void step();

    // Apply rules from config to the field
    void apply_rules(const GameConfig& cfg);

    // Find face index whose centroid is closest to the camera direction
    int front_face(const Camera3D& cam) const;

    int total_cells() const { return hex_count + pent_count; }

private:
    void recompute_centers();
    void recount_faces();
    void count_alive();
};
