#include "raylib.h"
#include "rlgl.h"
#include "raygui.h"
#include "game_world.h"
#include "cell_textures.h"
#include "cell_atlas.h"
#include "sim_panel.h"
#include "raymath.h"

#ifdef PLATFORM_WEB
#include <emscripten/emscripten.h>
#endif

struct AppState {
    SimPanel     panel;
    SimControls  controls;
    GameWorld    world;
    Camera3D     cam      = {};
    double       last_step = 0;
    float        dpr = 1.0f;
    Font         ui_font     = {};  // HUD overlay (Generation/Alive/FPS), 16*dpr
    Font         panel_font  = {};  // panel controls/labels, 16*UI_SCALE*dpr
    Font         credit_font = {};  // tech-credits line, 11*dpr
    CellTextures tex;
    CellAtlas    atlas;
};

#ifdef PLATFORM_WEB
// The canvas' on-screen (CSS) size is decided by the page layout — it flexes to
// fill the area left of the HTML control panel (see shell.html). Here we only
// keep the canvas backing store at physical-pixel resolution (CSS size * DPR)
// via SetWindowSize(), so RayLib renders crisp on HiDPI displays without the
// browser up/downscaling. We do NOT touch canvas.style — layout owns that.
// RayLib's FLAG_WINDOW_HIGHDPI is a no-op on web (see rcore_web.c), so this is
// how HiDPI is handled; GLFW's web shim scales mouse coords by the canvas/rect
// ratio, so input still lands correctly in GetScreenWidth()/GetMouseX() space.
static void SyncWebCanvasSize(AppState& s)
{
    static int last_css_w = -1, last_css_h = -1;
    static double last_dpr = -1.0;

    // clientWidth/Height = the canvas' laid-out CSS box (flex area).
    const int css_w = EM_ASM_INT({ return Module.canvas.clientWidth  || window.innerWidth;  });
    const int css_h = EM_ASM_INT({ return Module.canvas.clientHeight || window.innerHeight; });

    const double dpr = EM_ASM_DOUBLE({ return window.devicePixelRatio || 1.0; });

    if (css_w == last_css_w && css_h == last_css_h && dpr == last_dpr) return;
    last_css_w = css_w; last_css_h = css_h; last_dpr = dpr;
    s.dpr = (float)dpr;

    SetWindowSize((int)(css_w * dpr), (int)(css_h * dpr));
}
#endif

static void UpdateFrame(AppState& s)
{
#ifdef PLATFORM_WEB
    SyncWebCanvasSize(s);
#endif
    // ── Input ─────────────────────────────────────────────────────────
    if (IsKeyPressed(KEY_SPACE)) s.controls.cfg.paused = !s.controls.cfg.paused;
    if (IsKeyPressed(KEY_R)) {
        s.world.restart(s.controls.cfg, s.cam);
        s.last_step = GetTime();
    }

    s.world.apply_rules(s.controls.cfg);

    if (!s.controls.cfg.paused && GetTime() - s.last_step >= 1.0 / s.controls.cfg.speed) {
        s.world.step();
        s.world.update_render_mesh(s.atlas);
        s.last_step = GetTime();
    }

#ifdef PLATFORM_WEB
    // Web builds render only the 3D scene into the canvas; all controls and
    // stats live in the HTML panel beside it (see shell.html + the extern "C"
    // bridge below). So the canvas has no in-scene panel and no reserved width.
    const float actual_panel_width = 0.0f;
#else
    const float actual_panel_width = SimPanel::WIDTH * s.dpr;
#endif

    // ── Camera ────────────────────────────────────────────────────────
    const bool mouse_in_panel = GetMouseX() > GetScreenWidth() - actual_panel_width;

    // In auto-rotate, dragging on the 3D area drops into manual control,
    // resuming from the current orientation so the view doesn't jump.
    if (s.controls.is_orbital && !mouse_in_panel &&
        IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        Vector2 d = GetMouseDelta();
        if (d.x != 0.0f || d.y != 0.0f) {
            s.controls.cam_distance = Vector3Length(s.cam.position);
            s.controls.cam_pitch    = asinf(s.cam.position.y / s.controls.cam_distance) * RAD2DEG;
            s.controls.cam_yaw      = atan2f(s.cam.position.x, s.cam.position.z) * RAD2DEG;
            s.controls.is_orbital   = false;
        }
    }

    if (!s.controls.is_orbital && !mouse_in_panel) {
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            Vector2 delta = GetMouseDelta();
            s.controls.cam_yaw   -= delta.x * 0.3f;
            s.controls.cam_pitch += delta.y * 0.3f;
            if (s.controls.cam_pitch > 89.0f)  { s.controls.cam_pitch = 180.0f - s.controls.cam_pitch; s.controls.cam_yaw += 180.0f; }
            if (s.controls.cam_pitch < -89.0f) { s.controls.cam_pitch = -180.0f - s.controls.cam_pitch; s.controls.cam_yaw += 180.0f; }
        }
        float wheel = GetMouseWheelMove();
        if (wheel != 0.0f) {
#ifdef PLATFORM_WEB
            constexpr float WHEEL_SCALE = 0.08f;
#else
            constexpr float WHEEL_SCALE = 0.5f;
#endif
            s.controls.cam_distance = Clamp(s.controls.cam_distance - wheel * WHEEL_SCALE, 4.0f, 20.0f);
        }
    }
    if (!s.controls.is_orbital) {
        float yr = s.controls.cam_yaw * DEG2RAD, pr = s.controls.cam_pitch * DEG2RAD;
        s.cam.position = { s.controls.cam_distance * cosf(pr) * sinf(yr),
                           s.controls.cam_distance * sinf(pr),
                           s.controls.cam_distance * cosf(pr) * cosf(yr) };
    } else {
        UpdateCamera(&s.cam, CAMERA_ORBITAL);
    }

    BeginDrawing();
    ClearBackground(BLACK);

    // ── 3D — viewport restricted to area left of panel ────────────────
    // rlViewport / glViewport needs framebuffer (render) pixels.
    // GetScreenWidth returns logical pixels; GetRenderWidth returns device pixels.
    // On HiDPI/web these differ; using screen coords for GL viewport causes
    // the projection aspect ratio to mismatch the displayed area.
    //const float dpr    = (float)GetRenderWidth() / (float)GetScreenWidth();
    //const int   vp_w   = (int)((GetScreenWidth() - SimPanel::WIDTH) * dpr);
    const int   vp_w   = GetScreenWidth() - (int)actual_panel_width;
    const int   vp_h   = GetRenderHeight();
    rlViewport(0, 0, vp_w, vp_h);

    BeginMode3D(s.cam);

    // BeginMode3D uses full window for aspect ratio — override projection
    // so the sphere is centered in the 3D area, not the full window.
    rlMatrixMode(RL_PROJECTION);
    rlLoadIdentity();
    {
        // Aspect ratio in logical coords is identical to render coords (dpr cancels).
        const float aspect = (float)(GetScreenWidth() - actual_panel_width) / (float)GetScreenHeight();
        const float top    = RL_CULL_DISTANCE_NEAR * tanf(s.cam.fovy * 0.5f * DEG2RAD);
        rlFrustum(-top * aspect, top * aspect, -top, top,
                  RL_CULL_DISTANCE_NEAR, RL_CULL_DISTANCE_FAR);
    }
    rlMatrixMode(RL_MODELVIEW);

    constexpr float EDGE_LIFT = 1.001f;
    const int nfaces = (int)s.world.sphere->faces.size();

    // ── Pass 1: fill cells ────────────────────────────────────────────
    rlEnableBackfaceCulling();

    if (s.controls.use_textures && s.world.render_mesh_ready) {
        DrawMesh(s.world.render_mesh, s.world.render_material, MatrixIdentity());
    } else {
        for (int fi = 0; fi < nfaces; ++fi) {
            const HexFace& face = s.world.sphere->faces[fi];
            const int      n    = (int)face.verts.size();
            const Vector3& c    = s.world.face_centers[fi];
            const Color    col  = s.world.field->cells[fi].color;
            for (int i = 0; i < n; ++i)
                DrawTriangle3D(c,
                               s.world.sphere->verts[face.verts[(i+1)%n]],
                               s.world.sphere->verts[face.verts[i]], col);
        }
    }

    rlDisableBackfaceCulling();

    // ── Pass 2: edges — front-facing only ────────────────────────────
    auto draw_edges = [&](bool pent_only) {
        Color col = pent_only ? Color{80, 40, 0, 200} : Color{0, 0, 0, 200};
        for (int fi = 0; fi < nfaces; ++fi) {
            const HexFace& face = s.world.sphere->faces[fi];
            if (face.pentagon != pent_only) continue;
            const Vector3& c = s.world.face_centers[fi];
            if (Vector3DotProduct(c, Vector3Subtract(c, s.cam.position)) >= 0.0f)
                continue;
            const int n = (int)face.verts.size();
            for (int i = 0; i < n; ++i)
                DrawLine3D(
                    Vector3Scale(s.world.sphere->verts[face.verts[i]], EDGE_LIFT),
                    Vector3Scale(s.world.sphere->verts[face.verts[(i+1)%n]], EDGE_LIFT),
                    col);
        }
    };

    rlEnableSmoothLines();
    draw_edges(false);
    draw_edges(true);
    rlDisableSmoothLines();

    EndMode3D();

    // Restore full viewport for 2D overlay and panel
    rlViewport(0, 0, GetRenderWidth(), GetRenderHeight());

    // ── Canvas overlay (both platforms) ───────────────────────────────
    // On-scene HUD drawn over the canvas. On web the HTML panel duplicates
    // these stats; this overlay stays so the scene keeps its own counters.
    // Reuses the dpr-sized font.ttf loaded once in main().
    const int   font_size = (int)(16 * s.dpr);
    DrawRectangle(0, 0, (int)(240 * s.dpr), (int)(62 * s.dpr), Fade(BLACK, 0.55f));
    DrawTextPro(s.ui_font,
        TextFormat("Generation: %d", s.world.generation), {10.0f*s.dpr, 10.0f*s.dpr},{0,0},0,font_size,2,WHITE);
    DrawTextPro(s.ui_font,
        TextFormat("Alive: %d / %d", s.world.alive_count, s.world.total_cells()),
        {10.0f*s.dpr, 36.0f*s.dpr},{0,0},0,font_size,2,LIME);
    DrawTextPro(s.ui_font, TextFormat("%d FPS", GetFPS()),
        {GetScreenWidth() - actual_panel_width - (70 * s.dpr), 10 * s.dpr}, {0,0}, 0, font_size, 2, LIME);

#ifndef PLATFORM_WEB
    // ── UI panel (native only; web uses the HTML panel in shell.html) ──
    s.panel.draw(s.controls, s.cam, s.world, s.dpr, actual_panel_width);
#endif

    // Clear the flags after handling so a one-shot request runs exactly once.
    // The native raygui panel resets them at the top of its draw(); the web
    // JS panel does not, so we must clear them here for both — otherwise a JS
    // request stays true and re-fires every frame (e.g. restart reseeding from
    // front_face(cam) each frame, making cells jump as the camera rotates).
    if (s.controls.restart_requested) {
        s.world.restart(s.controls.cfg, s.cam);
        s.world.update_render_mesh(s.atlas);
        s.last_step = GetTime();
        s.controls.restart_requested = false;
    }
    if (s.controls.rebuild_requested) {
        s.world.rebuild(static_cast<int>(s.controls.cfg.subdiv));
        s.world.restart(s.controls.cfg, s.cam);
        s.world.build_render_mesh(s.atlas);
        s.last_step = GetTime();
        s.controls.rebuild_requested = false;
    }

    EndDrawing();
}

#ifdef PLATFORM_WEB
static AppState* g_app;
static void WasmFrame() { UpdateFrame(*g_app); }

// ── JS ↔ engine bridge ────────────────────────────────────────────────────
// The HTML/JS panel (shell.html) reads and writes ONLY these functions, which
// forward to g_app->controls (UI-writable state) and g_app->world (read-only
// derived state). The engine itself never knows a JS panel exists — this is the
// same SimControls boundary the native raygui panel uses, exposed to the web.
extern "C" {

// True once main() has created g_app; the JS panel waits for this before
// calling any other getter/setter (they dereference g_app). Safe to call
// early — it only null-checks, never dereferences.
EMSCRIPTEN_KEEPALIVE int ui_ready() { return g_app != nullptr ? 1 : 0; }

// — Setters: UI → controls —
EMSCRIPTEN_KEEPALIVE void ui_set_paused(int v)    { g_app->controls.cfg.paused = (v != 0); }
EMSCRIPTEN_KEEPALIVE void ui_set_speed(float v)   { g_app->controls.cfg.speed = v; }
EMSCRIPTEN_KEEPALIVE void ui_set_seed_size(int v) { g_app->controls.cfg.seed_size = (SeedSize)v; }
EMSCRIPTEN_KEEPALIVE void ui_set_rules(int v)     { g_app->controls.cfg.rules = (Rules)v; }
EMSCRIPTEN_KEEPALIVE void ui_set_textures(int v)  { g_app->controls.use_textures = (v != 0); }
EMSCRIPTEN_KEEPALIVE void ui_restart()            { g_app->controls.restart_requested = true; }

EMSCRIPTEN_KEEPALIVE void ui_set_subdiv(int v) {
    g_app->controls.cfg.subdiv = (Subdiv)v;
    g_app->controls.rebuild_requested = true;
}

// Mirrors SimPanel's camera toggle: when leaving orbital mode, capture the
// current orientation back into controls so manual control resumes smoothly.
EMSCRIPTEN_KEEPALIVE void ui_set_orbital(int v) {
    SimControls& c = g_app->controls;
    const bool want = (v != 0);
    if (c.is_orbital && !want) {
        c.cam_distance = Vector3Length(g_app->cam.position);
        c.cam_pitch    = asinf(g_app->cam.position.y / c.cam_distance) * RAD2DEG;
        c.cam_yaw      = atan2f(g_app->cam.position.x, g_app->cam.position.z) * RAD2DEG;
    }
    c.is_orbital = want;
}

// — Getters: controls / world → UI —
EMSCRIPTEN_KEEPALIVE int   ui_get_paused()     { return g_app->controls.cfg.paused ? 1 : 0; }
EMSCRIPTEN_KEEPALIVE float ui_get_speed()      { return g_app->controls.cfg.speed; }
EMSCRIPTEN_KEEPALIVE int   ui_get_seed_size()  { return (int)g_app->controls.cfg.seed_size; }
EMSCRIPTEN_KEEPALIVE int   ui_get_seed_count() { return g_app->controls.cfg.seed_count(); }
EMSCRIPTEN_KEEPALIVE int   ui_get_rules()      { return (int)g_app->controls.cfg.rules; }
EMSCRIPTEN_KEEPALIVE int   ui_get_subdiv()     { return (int)g_app->controls.cfg.subdiv; }
EMSCRIPTEN_KEEPALIVE int   ui_get_orbital()    { return g_app->controls.is_orbital ? 1 : 0; }
EMSCRIPTEN_KEEPALIVE int   ui_get_textures()   { return g_app->controls.use_textures ? 1 : 0; }

EMSCRIPTEN_KEEPALIVE int ui_get_generation() { return g_app->world.generation; }
EMSCRIPTEN_KEEPALIVE int ui_get_alive()      { return g_app->world.alive_count; }
EMSCRIPTEN_KEEPALIVE int ui_get_total()      { return g_app->world.total_cells(); }
EMSCRIPTEN_KEEPALIVE int ui_get_hex()        { return g_app->world.hex_count; }
EMSCRIPTEN_KEEPALIVE int ui_get_pent()       { return g_app->world.pent_count; }
EMSCRIPTEN_KEEPALIVE int ui_get_fps()        { return GetFPS(); }

// Live rule thresholds (for the "Born / Survives" explanation text).
EMSCRIPTEN_KEEPALIVE int ui_get_rule_b_lo() { return g_app->world.field->rule_b_lo; }
EMSCRIPTEN_KEEPALIVE int ui_get_rule_b_hi() { return g_app->world.field->rule_b_hi; }
EMSCRIPTEN_KEEPALIVE int ui_get_rule_s_lo() { return g_app->world.field->rule_s_lo; }
EMSCRIPTEN_KEEPALIVE int ui_get_rule_s_hi() { return g_app->world.field->rule_s_hi; }

} // extern "C"
#endif


int main()
{
#ifdef PLATFORM_WEB
    // No FLAG_WINDOW_RESIZABLE: RayLib's own auto-resize re-sizes the canvas in
    // CSS pixels, which would fight SyncWebCanvasSize(). Resize is handled
    // manually every frame instead (see SyncWebCanvasSize / UpdateFrame).
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    // Size the backing store from the canvas' laid-out CSS box (flex area beside
    // the HTML panel); SyncWebCanvasSize keeps it in sync every frame after.
    const int   cssW = EM_ASM_INT({ return Module.canvas.clientWidth  || window.innerWidth;  });
    const int   cssH = EM_ASM_INT({ return Module.canvas.clientHeight || window.innerHeight; });
    const double dpr = EM_ASM_DOUBLE({ return window.devicePixelRatio || 1.0; });
    InitWindow((int)(cssW * dpr), (int)(cssH * dpr), "Hex Sphere — Game of Life");
#else
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
    InitWindow(1500, 800, "Hex Sphere — Game of Life");
#endif

    SetTargetFPS(60);

    static AppState app;

#ifndef PLATFORM_WEB
    app.dpr = GetWindowScaleDPI().x;
#else
    app.dpr = (float)dpr;
#endif

    // Each font is rasterized at the exact size it's drawn at (see LoadUIFont)
    // so raylib's point-filtered atlas is never up/downscaled and glyphs stay
    // crisp. ui_font is loaded independently of raygui so the canvas overlay
    // (below) can draw with it directly instead of going through GuiGetFont().
    app.ui_font     = LoadUIFont(app.dpr, 16.0f);
    app.panel_font  = LoadUIFont(app.dpr, 16.0f * SimPanel::UI_SCALE);
    app.credit_font = LoadUIFont(app.dpr, 11.0f);
    app.panel.init(app.panel_font, app.credit_font);

    app.cam.position   = {0.0f, 0.0f, 9.0f};
    app.cam.target     = {0.0f, 0.0f, 0.0f};
    app.cam.up         = {0.0f, 1.0f, 0.0f};
    app.cam.fovy       = 45.0f;
    app.cam.projection = CAMERA_PERSPECTIVE;
    app.last_step      = GetTime();

    app.world.rebuild(static_cast<int>(app.controls.cfg.subdiv));
    app.world.restart(app.controls.cfg, {0.0f, 0.0f, 9.0f});

    app.tex.load();
    app.atlas.load();
    app.world.build_render_mesh(app.atlas);

#ifdef PLATFORM_WEB
    g_app = &app;
    emscripten_set_main_loop(WasmFrame, 0, 1);
#else
    while (!WindowShouldClose())
        UpdateFrame(app);

    app.world.unload_render_mesh();
    app.tex.unload();
    app.atlas.unload();
    // Only unload fonts that are custom — never unload raylib's built-in
    // default font, which LoadUIFont() falls back to and which raylib itself
    // owns for the lifetime of the app.
    for (Font f : { app.ui_font, app.panel_font, app.credit_font })
        if (f.texture.id != GetFontDefault().texture.id)
            UnloadFont(f);
    CloseWindow();
#endif
    return 0;
}
