#pragma once

#include "raylib.h"
#include "game_config.h"

// Loads the shared UI font (res/font.ttf) at a dpr-scaled size, independent
// of raygui — callers keep the Font themselves instead of relying on
// GuiGetFont(), so it can be reused for plain DrawText* calls too.
// Falls back to raylib's built-in font if the file isn't found.
Font LoadUIFont(float dpr);

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

    // Call once after InitWindow with the font from LoadUIFont().
    void init(Font font);

    // Draw the panel. cfg is read/written by UI controls.
    // cam/dist/pitch/yaw are modified by the camera toggle button.
    void draw(GameConfig& cfg, Camera3D& cam,
              float& dist, float& pitch, float& yaw, float dpr, float panel_width);
};
