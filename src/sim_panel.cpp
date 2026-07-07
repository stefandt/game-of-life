#include "sim_panel.h"
#include "game_world.h"
#include "raygui.h"
#include "raymath.h"

Font LoadUIFont(float dpr, float base_size)
{
    const int font_size = (int)(base_size * dpr + 0.5f);

    // Bundled in res/ (preloaded into the WASM virtual FS too) so desktop and
    // web render the identical font instead of falling back to raylib's
    // low-res built-in bitmap font when OS system fonts aren't reachable.
    if (FileExists("res/font.ttf"))
        return LoadFontEx("res/font.ttf", font_size, nullptr, 0);
    return GetFontDefault();
}

void SimPanel::init(Font panel_font, Font credit_font)
{
    GuiSetFont(panel_font);
    credit_font_ = credit_font;

    // raygui's default light theme uses mid-gray text (0x686868) on mid-gray
    // control fills (0xc9c9c9, e.g. buttons/sliders/comboboxes) — contrast
    // ratio ~3.3:1, reads as "gray on gray". Darken text across all states
    // so it stands out against both the panel background and control fills.
    GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL,   0x1f1f1fff);
    GuiSetStyle(DEFAULT, TEXT_COLOR_FOCUSED,  0x0f4c68ff);
    GuiSetStyle(DEFAULT, TEXT_COLOR_PRESSED,  0x043e52ff);
    GuiSetStyle(DEFAULT, BORDER_COLOR_NORMAL, 0x4d4d4dff);
}

void SimPanel::draw(SimControls& controls, const Camera3D& cam, const GameWorld& world,
                    float dpr, float panel_width)
{
    GameConfig& cfg = controls.cfg;

    controls.restart_requested = false;
    controls.rebuild_requested = false;

    const float px = (float)GetScreenWidth() - panel_width;
    const float iw = panel_width - (30.0f * dpr);
    const float ix = px + (15.0f * dpr);
    const float ih = 28.0f * UI_SCALE * dpr;
    const float sp = 10.0f * UI_SCALE * dpr;
    float y = 34.0f * UI_SCALE * dpr;

    // GuiSlider draws its min/max text labels outside the given bounds, to
    // the left and right of the track — inset the track itself so those
    // labels land inside the panel instead of overhanging its edges.
    const float slider_inset = 26.0f * UI_SCALE * dpr;
    const float sx = ix + slider_inset;
    const float sw = iw - 2.0f * slider_inset;

    // scale style Raygui — matches the size panel_font (see init()) was
    // rasterized at, so glyphs aren't drawn at a different size than loaded.
    GuiSetStyle(DEFAULT, TEXT_SIZE, (int)(16.0f * UI_SCALE * dpr));
    GuiSetStyle(SLIDER,  SLIDER_WIDTH, (int)(16.0f * UI_SCALE * dpr));
    GuiSetStyle(SLIDER,  SLIDER_PADDING, (int)(30.0f * UI_SCALE * dpr));

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
            controls.rebuild_requested = true;
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
    GuiSlider({sx, y, sw, ih}, "0.25", "8", &cfg.speed, 0.25f, 8.0f);
    y += ih + sp;

    // ── Initial cells (snap slider) ───────────────────────────────────────
    GuiLabel({ix, y, iw, ih},
        TextFormat("Initial cells: %d", cfg.seed_count()));
    y += ih;
    {
        float seed_f = (float)cfg.seed_size;
        GuiSlider({sx, y, sw, ih}, "10", "2k", &seed_f, 0.0f, 7.0f);
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

    // ── Rule explanation — reads world.field's live thresholds directly so
    // it can never drift out of sync with the rule actually being applied.
    {
        const GameField& f = *world.field;
        if (f.rule_b_lo == f.rule_b_hi)
            GuiLabel({ix, y, iw, ih}, TextFormat("Born: exactly %d neighbors", f.rule_b_lo));
        else
            GuiLabel({ix, y, iw, ih}, TextFormat("Born: %d-%d neighbors", f.rule_b_lo, f.rule_b_hi));
        y += ih;
        GuiLabel({ix, y, iw, ih}, TextFormat("Survives: %d-%d neighbors", f.rule_s_lo, f.rule_s_hi));
        y += ih;
        GuiLabel({ix, y, iw, ih}, "Dies: otherwise");
    }
    y += ih + sp;

    // ── Buttons ───────────────────────────────────────────────────────────
    if (GuiButton({ix, y, iw, ih}, cfg.paused ? "#131# Resume" : "#132# Pause"))
        cfg.paused = !cfg.paused;
    y += ih + sp;

    if (GuiButton({ix, y, iw, ih}, "#76# Restart"))
        controls.restart_requested = true;
    y += ih + sp;

    GuiLine({ix, y, iw, 1}, nullptr); y += sp + 4;

    // ── Camera ────────────────────────────────────────────────────────────
    if (GuiButton({ix, y, iw, ih},
            controls.is_orbital ? "#65# Mouse Orbit" : "#65# Auto-rotate")) {
        if (controls.is_orbital) {
            controls.cam_distance = Vector3Length(cam.position);
            controls.cam_pitch    = asinf(cam.position.y / controls.cam_distance) * RAD2DEG;
            controls.cam_yaw      = atan2f(cam.position.x, cam.position.z) * RAD2DEG;
            controls.is_orbital = false;
        } else {
            controls.is_orbital = true;
        }
    }
    y += ih + sp;

    GuiLine({ix, y, iw, 1}, nullptr); y += sp + 4;

    // ── Render mode ───────────────────────────────────────────────────────
    GuiCheckBox({ix, y, ih, ih}, "  Textures", &controls.use_textures);
    y += ih + sp;

    GuiLine({ix, y, iw, 1}, nullptr); y += sp + 4;
    GuiLabel({ix, y, iw, ih},
        TextFormat("%d hex   %d pent", world.hex_count, world.pent_count));
    y += ih + sp;
    GuiLabel({ix, y, iw, ih},
        TextFormat("Alive: %d / %d", world.alive_count, world.total_cells()));

    // ── Tech credits ─────────────────────────────────────────────────────
    // Anchored to the bottom of the screen (not the scrolling 'y' cursor
    // above) so it stays put regardless of how much control content exists.
    // Uses credit_font_, rasterized natively at this size — reusing the main
    // (larger) UI font and drawing it smaller would downscale a point-filtered
    // atlas and blur the glyphs.
    {
#ifdef PLATFORM_WEB
        static const char* kLines[] = {
            "C++20",
            "OpenMesh: geodesic dual mesh",
            "(icosahedron -> hex/pentagon)",
            "RayLib: 3D render + GUI",
            "WASM build via Emscripten",
        };
#else
        static const char* kLines[] = {
            "C++20",
            "OpenMesh: geodesic dual mesh",
            "(icosahedron -> hex/pentagon)",
            "RayLib: 3D render + GUI",
        };
#endif
        const int   line_count  = sizeof(kLines) / sizeof(kLines[0]);
        const int   credit_size = (int)(11.0f * dpr + 0.5f);
        const float line_h      = credit_size + 4.0f * dpr;
        const Color dim         = Color{150, 150, 150, 255};

        float cy = (float)GetScreenHeight() - line_count * line_h - (10.0f * dpr);
        for (int i = 0; i < line_count; ++i) {
            DrawTextEx(credit_font_, kLines[i], {ix, cy}, (float)credit_size, 1.0f, dim);
            cy += line_h;
        }
    }
}
