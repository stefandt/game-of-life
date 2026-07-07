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
    Font         ui_font  = {};
    CellTextures tex;
    CellAtlas    atlas;
};

#ifdef PLATFORM_WEB
// RayLib's web backend sizes the canvas in CSS pixels (FLAG_WINDOW_HIGHDPI is a
// no-op there — see rcore_web.c), so on any display with devicePixelRatio != 1
// the browser has to upscale the whole canvas, blurring/aliasing both the 3D
// scene and the UI. Fix without touching vendored RayLib: keep the canvas
// backing store at physical-pixel resolution ourselves (CSS size * DPR) via
// SetWindowSize(), then reset the CSS display size back to logical pixels —
// GLFW's web shim scales mouse coordinates by canvas/rect ratio, so clicks
// still land correctly against GetScreenWidth()/GetMouseX() in this space.
static void SyncWebCanvasSize(AppState& s)
{
    static int last_css_w = -1, last_css_h = -1;
    static double last_dpr = -1.0;

    const int css_w = EM_ASM_INT({ return window.innerWidth;  });
    const int css_h = EM_ASM_INT({ return window.innerHeight; });

    const double dpr = EM_ASM_DOUBLE({ return window.devicePixelRatio || 1.0; });

    if (css_w == last_css_w && css_h == last_css_h && dpr == last_dpr) return;
    last_css_w = css_w; last_css_h = css_h; last_dpr = dpr;
    s.dpr = (float)dpr;
    
    SetWindowSize((int)(css_w * dpr), (int)(css_h * dpr));

    EM_ASM({
        Module.canvas.style.width  = $0 + 'px';
        Module.canvas.style.height = $1 + 'px';
    }, css_w, css_h);
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

    const float actual_panel_width = SimPanel::WIDTH * s.dpr;

    // ── Camera ────────────────────────────────────────────────────────
    const bool mouse_in_panel = GetMouseX() > GetScreenWidth() - actual_panel_width;

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

    // ── Canvas overlay ────────────────────────────────────────────────
    // Reuses the dpr-sized font.ttf loaded once in main() (s.ui_font), instead
    // of raylib's low-res built-in bitmap font (GetFontDefault()) which looked
    // pixelated once scaled up to font_size.
    const int   font_size = (int)(16 * s.dpr);
    DrawRectangle(0, 0, (int)(240 * s.dpr), (int)(62 * s.dpr), Fade(BLACK, 0.55f));
    DrawTextPro(s.ui_font,
        TextFormat("Generation: %d", s.world.generation), {10.0f*s.dpr, 10.0f*s.dpr},{0,0},0,font_size,2,WHITE);
    DrawTextPro(s.ui_font,
        TextFormat("Alive: %d / %d", s.world.alive_count, s.world.total_cells()),
        {10.0f*s.dpr, 36.0f*s.dpr},{0,0},0,font_size,2,LIME);
    DrawTextPro(s.ui_font, TextFormat("%d FPS", GetFPS()),
        {GetScreenWidth() - actual_panel_width - (70 * s.dpr), 10 * s.dpr}, {0,0}, 0, font_size, 2, LIME);

    // ── UI panel ──────────────────────────────────────────────────────
    s.panel.draw(s.controls, s.cam, s.world, s.dpr, actual_panel_width);

    if (s.controls.restart_requested) {
        s.world.restart(s.controls.cfg, s.cam);
        s.world.update_render_mesh(s.atlas);
        s.last_step = GetTime();
    }
    if (s.controls.rebuild_requested) {
        s.world.rebuild(static_cast<int>(s.controls.cfg.subdiv));
        s.world.restart(s.controls.cfg, s.cam);
        s.world.build_render_mesh(s.atlas);
        s.last_step = GetTime();
    }

    EndDrawing();
}

#ifdef PLATFORM_WEB
static AppState* g_app;
static void WasmFrame() { UpdateFrame(*g_app); }
#endif


int main()
{
#ifdef PLATFORM_WEB
    // No FLAG_WINDOW_RESIZABLE: RayLib's own auto-resize re-sizes the canvas in
    // CSS pixels, which would fight SyncWebCanvasSize(). Resize is handled
    // manually every frame instead (see SyncWebCanvasSize / UpdateFrame).
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    const int   cssW = EM_ASM_INT({ return window.innerWidth;  });
    const int   cssH = EM_ASM_INT({ return window.innerHeight; });
    const double dpr = EM_ASM_DOUBLE({ return window.devicePixelRatio || 1.0; });
    InitWindow((int)(cssW * dpr), (int)(cssH * dpr), "Hex Sphere — Game of Life");
    EM_ASM({
        Module.canvas.style.width  = $0 + 'px';
        Module.canvas.style.height = $1 + 'px';
    }, cssW, cssH);
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

    // Font is rasterized at dpr-scaled size up front — GuiSetStyle(TEXT_SIZE)
    // in SimPanel::draw() only ever scales it down from here, never up, so
    // glyphs stay crisp instead of being blurrily upscaled from a 20px bitmap.
    // Loaded independently of raygui so the canvas overlay (below) can reuse
    // it directly instead of going through GuiGetFont().
    app.ui_font = LoadUIFont(app.dpr);
    app.panel.init(app.ui_font);

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
    // Only unload if it's the custom font — never unload raylib's built-in
    // default font, which LoadUIFont() falls back to and which raylib itself
    // owns for the lifetime of the app.
    if (app.ui_font.texture.id != GetFontDefault().texture.id)
        UnloadFont(app.ui_font);
    CloseWindow();
#endif
    return 0;
}
