#include "sim_panel.h"
#include "raygui.h"
#include "raymath.h"

void SimPanel::init()
{
    Font font = GetFontDefault();
    // Try system fonts in order: Windows → macOS → Linux fallback
    if      (FileExists("C:/Windows/Fonts/segoeui.ttf"))
        font = LoadFontEx("C:/Windows/Fonts/segoeui.ttf", 20, nullptr, 0);
    else if (FileExists("/Library/Fonts/Arial.ttf"))
        font = LoadFontEx("/Library/Fonts/Arial.ttf", 20, nullptr, 0);
    else if (FileExists("/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf"))
        font = LoadFontEx("/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf", 20, nullptr, 0);
    GuiSetFont(font);
    // GuiSetStyle(DEFAULT, TEXT_SIZE,      20);
    // GuiSetStyle(SLIDER,  SLIDER_WIDTH,   16);
    // GuiSetStyle(SLIDER,  SLIDER_PADDING,  30);
}

void SimPanel::draw(GameConfig& cfg, Camera3D& cam,
                    float& dist, float& pitch, float& yaw, float dpr, float panel_width)
{
    restart_requested = false;
    rebuild_requested = false;

    const float px = (float)GetScreenWidth() - panel_width;
    const float iw = panel_width - (30.0f * dpr);
    const float ix = px + (15.0f * dpr);
    const float ih = 28.0f * dpr;
    const float sp = 10.0f * dpr;
    float y = 34.0f * dpr;

    // scale style Raygui
    GuiSetStyle(DEFAULT, TEXT_SIZE, (int)(20.0f * dpr));
    GuiSetStyle(SLIDER,  SLIDER_WIDTH, (int)(16.0f * dpr));
    GuiSetStyle(SLIDER,  SLIDER_PADDING, (int)(30.0f * dpr));

    GuiPanel({px, 0, panel_width, (float)GetScreenHeight()}, "Simulation");

    // ── Init / map settings ───────────────────────────────────────────────
    GuiLabel({ix, y, iw, ih}, "Subdivisions (Cells):");
    y += ih;
    {
        // ComboBox index 0-3 → Subdiv value 2-5
        int idx = cfg.subdiv - 2;
        const int prev = idx;
        GuiToggleSlider({ix, y, iw, ih}, "2 (162);3 (642);4 (2562);5 (10242)", &idx);
        if (idx != prev) {
            cfg.subdiv = (Subdiv)(idx + 2);
            rebuild_requested = true;
        }
    }
    y += ih + sp;

    GuiLine({ix, y, iw, 1}, nullptr); y += sp + 4;

    // ── Status ────────────────────────────────────────────────────────────
    GuiLabel({ix, y, iw, ih},
        cfg.paused ? "#132# Paused" : "#131# Running");
    y += ih + sp;

    // ── Speed ─────────────────────────────────────────────────────────────
    GuiLabel({ix, y, iw, ih},
        TextFormat("Speed: %.2f steps/s", cfg.speed));
    y += ih;
    GuiSlider({ix, y, iw, ih}, "0.25", "8", &cfg.speed, 0.25f, 8.0f);
    y += ih + sp;

    // ── Initial cells (snap slider) ───────────────────────────────────────
    GuiLabel({ix, y, iw, ih},
        TextFormat("Initial cells: %d", cfg.seed_count()));
    y += ih;
    {
        float seed_f = (float)cfg.seed_size;
        GuiSlider({ix, y, iw, ih}, "10", "2k", &seed_f, 0.0f, 7.0f);
        cfg.seed_size = (SeedSize)Clamp((int)(seed_f + 0.5f), 0, 7);
    }
    y += ih + sp;

    // ── Rules ─────────────────────────────────────────────────────────────
    GuiLabel({ix, y, iw, ih}, "Rules:");
    y += ih;
    {
        int rules_int = cfg.rules;
        GuiComboBox({ix, y, iw, ih}, "B2/S23;B3/S23;B2/S34", &rules_int);
        cfg.rules = (Rules)rules_int;
    }
    y += ih + sp;

    // ── Buttons ───────────────────────────────────────────────────────────
    if (GuiButton({ix, y, iw, ih}, cfg.paused ? "#131# Resume" : "#132# Pause"))
        cfg.paused = !cfg.paused;
    y += ih + sp;

    if (GuiButton({ix, y, iw, ih}, "#76# Restart"))
        restart_requested = true;
    y += ih + sp;

    GuiLine({ix, y, iw, 1}, nullptr); y += sp + 4;

    // ── Camera ────────────────────────────────────────────────────────────
    if (GuiButton({ix, y, iw, ih},
            is_orbital ? "#65# Mouse Orbit" : "#65# Auto-rotate")) {
        if (is_orbital) {
            dist  = Vector3Length(cam.position);
            pitch = asinf(cam.position.y / dist) * RAD2DEG;
            yaw   = atan2f(cam.position.x, cam.position.z) * RAD2DEG;
            is_orbital = false;
        } else {
            is_orbital = true;
        }
    }
    y += ih + sp;

    GuiLine({ix, y, iw, 1}, nullptr); y += sp + 4;

    // ── Render mode ───────────────────────────────────────────────────────
    GuiCheckBox({ix, y, ih, ih}, "  Textures", &use_textures);
    y += ih + sp;

    GuiLine({ix, y, iw, 1}, nullptr); y += sp + 4;
    GuiLabel({ix, y, iw, ih},
        TextFormat("%d hex   %d pent", hex_count, pent_count));
    y += ih + sp;
    GuiLabel({ix, y, iw, ih},
        TextFormat("Alive: %d / %d", alive_count, total_cells));
}
