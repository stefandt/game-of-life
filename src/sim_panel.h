#pragma once

#include "raylib.h"
#include "game_config.h"

// SimPanel owns only UI-specific state (camera preference, action flags).
// Game settings live in GameConfig and are passed by reference to draw().
struct SimPanel {
    // ── UI-only state ─────────────────────────────────────────────────────
    bool is_orbital   = false;
    bool use_textures = true;

    // ── Action flags — set true when user clicks, main clears them ────────
    bool restart_requested = false;
    bool rebuild_requested = false;

    // ── Display info — filled by caller each frame ────────────────────────
    int generation  = 0;
    int alive_count = 0;
    int total_cells = 0;
    int hex_count   = 0;
    int pent_count  = 0;

    static constexpr int WIDTH = 320;

    // Call once after InitWindow
    void init();

    // Draw the panel. cfg is read/written by UI controls.
    // cam/dist/pitch/yaw are modified by the camera toggle button.
    void draw(GameConfig& cfg, Camera3D& cam,
              float& dist, float& pitch, float& yaw);
};
