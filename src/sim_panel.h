#pragma once

#include "raylib.h"
#include "sim_controls.h"

struct GameWorld;

// Loads the shared UI font (res/font.ttf) rasterized at base_size * dpr,
// independent of raygui — callers keep the Font themselves instead of
// relying on GuiGetFont(), so it can be reused for plain DrawText* calls too.
// Falls back to raylib's built-in font if the file isn't found.
// Pass the exact on-screen size a caller will draw at — scaling a Font's
// glyphs down (or up) from a different rasterized size blurs them, since
// raylib's font atlas uses point-filtered sampling.
Font LoadUIFont(float dpr, float base_size = 16.0f);

// SimPanel renders SimControls/GameWorld; the fonts it holds are rendering
// resources, not UI or control state.
struct SimPanel {
    static constexpr int   WIDTH    = 320;
    // Panel controls/labels are drawn at 16 * UI_SCALE to sit less oversized
    // next to the 3D view. Callers should load the panel font at that same
    // size (see LoadUIFont's blur note) instead of downscaling a 16px font.
    static constexpr float UI_SCALE = 0.75f;

    // Call once after InitWindow. panel_font (sized 16 * UI_SCALE * dpr, see
    // UI_SCALE) is used for controls/labels; credit_font (sized ~11 * dpr) is
    // for the tech credits line at the bottom — kept separate so neither is
    // drawn at a size other than the one it was rasterized at.
    void init(Font panel_font, Font credit_font);

    // Draw the panel. controls is read/written by UI actions; cam and world
    // are read-only (cam is needed to capture orientation when leaving
    // orbital mode; world supplies generation/alive/hex/pent counts).
    void draw(SimControls& controls, const Camera3D& cam, const GameWorld& world,
              float dpr, float panel_width);

private:
    Font credit_font_ = {};
};
