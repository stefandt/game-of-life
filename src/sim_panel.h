#pragma once

#include "raylib.h"
#include "sim_controls.h"

struct GameWorld;

// Loads the shared UI font (res/font.ttf) at a dpr-scaled size, independent
// of raygui — callers keep the Font themselves instead of relying on
// GuiGetFont(), so it can be reused for plain DrawText* calls too.
// Falls back to raylib's built-in font if the file isn't found.
Font LoadUIFont(float dpr);

// SimPanel is a stateless renderer: it only reads/writes SimControls and
// GameWorld, holding no UI or display state of its own.
struct SimPanel {
    static constexpr int WIDTH = 320;

    // Call once after InitWindow with the font from LoadUIFont().
    void init(Font font);

    // Draw the panel. controls is read/written by UI actions; cam and world
    // are read-only (cam is needed to capture orientation when leaving
    // orbital mode; world supplies generation/alive/hex/pent counts).
    void draw(SimControls& controls, const Camera3D& cam, const GameWorld& world,
              float dpr, float panel_width);
};
