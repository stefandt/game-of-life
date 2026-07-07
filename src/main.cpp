#include "raylib.h"
#include "rlgl.h"
#include "raygui.h"
#include "game_config.h"
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
    GameConfig   config;
    GameWorld    world;
    Camera3D     cam      = {};
    float        yaw = 0, pitch = 20, distance = 9;
    double       last_step = 0;
    CellTextures tex;
    CellAtlas    atlas;
};

static void UpdateFrame(AppState& s)
{
    // ── Input ─────────────────────────────────────────────────────────
    if (IsKeyPressed(KEY_SPACE)) s.config.paused = !s.config.paused;
    if (IsKeyPressed(KEY_R)) {
        s.world.restart(s.config, s.cam);
        s.last_step = GetTime();
    }

    s.world.apply_rules(s.config);

    if (!s.config.paused && GetTime() - s.last_step >= 1.0 / s.config.speed) {
        s.world.step();
        s.world.update_render_mesh(s.atlas);
        s.last_step = GetTime();
    }

    // ── Camera ────────────────────────────────────────────────────────
    const bool mouse_in_panel = GetMouseX() > GetScreenWidth() - SimPanel::WIDTH;

    if (!s.panel.is_orbital && !mouse_in_panel) {
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            Vector2 delta = GetMouseDelta();
            s.yaw   -= delta.x * 0.3f;
            s.pitch += delta.y * 0.3f;
            if (s.pitch > 89.0f)  { s.pitch = 180.0f - s.pitch; s.yaw += 180.0f; }
            if (s.pitch < -89.0f) { s.pitch = -180.0f - s.pitch; s.yaw += 180.0f; }
        }
        float wheel = GetMouseWheelMove();
        if (wheel != 0.0f) {
#ifdef PLATFORM_WEB
            constexpr float WHEEL_SCALE = 0.08f;
#else
            constexpr float WHEEL_SCALE = 0.5f;
#endif
            s.distance = Clamp(s.distance - wheel * WHEEL_SCALE, 4.0f, 20.0f);
        }
    }
    if (!s.panel.is_orbital) {
        float yr = s.yaw * DEG2RAD, pr = s.pitch * DEG2RAD;
        s.cam.position = { s.distance * cosf(pr) * sinf(yr),
                           s.distance * sinf(pr),
                           s.distance * cosf(pr) * cosf(yr) };
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
    const float dpr    = (float)GetRenderWidth() / (float)GetScreenWidth();
    const int   vp_w   = (int)((GetScreenWidth() - SimPanel::WIDTH) * dpr);
    const int   vp_h   = GetRenderHeight();
    rlViewport(0, 0, vp_w, vp_h);

    BeginMode3D(s.cam);

    // BeginMode3D uses full window for aspect ratio — override projection
    // so the sphere is centered in the 3D area, not the full window.
    rlMatrixMode(RL_PROJECTION);
    rlLoadIdentity();
    {
        // Aspect ratio in logical coords is identical to render coords (dpr cancels).
        const float aspect = (float)(GetScreenWidth() - SimPanel::WIDTH) / (float)GetScreenHeight();
        const float top    = RL_CULL_DISTANCE_NEAR * tanf(s.cam.fovy * 0.5f * DEG2RAD);
        rlFrustum(-top * aspect, top * aspect, -top, top,
                  RL_CULL_DISTANCE_NEAR, RL_CULL_DISTANCE_FAR);
    }
    rlMatrixMode(RL_MODELVIEW);

    constexpr float EDGE_LIFT = 1.001f;
    const int nfaces = (int)s.world.sphere->faces.size();

    // ── Pass 1: fill cells ────────────────────────────────────────────
    rlEnableBackfaceCulling();

    if (s.panel.use_textures && s.world.render_mesh_ready) {
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
    DrawRectangle(0, 0, 240, 62, Fade(BLACK, 0.55f));
    DrawTextPro(GetFontDefault(),
        TextFormat("Generation: %d", s.world.generation), {10,10},{0,0},0,20,2,WHITE);
    DrawTextPro(GetFontDefault(),
        TextFormat("Alive: %d / %d", s.world.alive_count, s.world.total_cells()),
        {10,36},{0,0},0,20,2,LIME);
    DrawText(TextFormat("%d FPS", GetFPS()),
             GetScreenWidth() - SimPanel::WIDTH - 70, 10, 20, LIME);

    // ── UI panel ──────────────────────────────────────────────────────
    s.panel.generation  = s.world.generation;
    s.panel.alive_count = s.world.alive_count;
    s.panel.total_cells = s.world.total_cells();
    s.panel.hex_count   = s.world.hex_count;
    s.panel.pent_count  = s.world.pent_count;

    s.panel.draw(s.config, s.cam, s.distance, s.pitch, s.yaw);

    if (s.panel.restart_requested) {
        s.world.restart(s.config, s.cam);
        s.world.update_render_mesh(s.atlas);
        s.last_step = GetTime();
    }
    if (s.panel.rebuild_requested) {
        s.world.rebuild(static_cast<int>(s.config.subdiv));
        s.world.restart(s.config, s.cam);
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
    // HIGHDPI: canvas attribute = viewport * devicePixelRatio (crisp rendering).
    // RESIZABLE: RayLib updates canvas + GetScreenWidth() via Emscripten callback
    //            on every browser resize — no CSS/attribute desync.
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_HIGHDPI | FLAG_WINDOW_RESIZABLE);
    int initW = EM_ASM_INT({ return window.innerWidth;  });
    int initH = EM_ASM_INT({ return window.innerHeight; });
    InitWindow(initW, initH, "Hex Sphere — Game of Life");
#else
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
    InitWindow(1500, 800, "Hex Sphere — Game of Life");
#endif
    SetTargetFPS(60);

    static AppState app;
    app.panel.init();

    app.cam.position   = {0.0f, 0.0f, 9.0f};
    app.cam.target     = {0.0f, 0.0f, 0.0f};
    app.cam.up         = {0.0f, 1.0f, 0.0f};
    app.cam.fovy       = 45.0f;
    app.cam.projection = CAMERA_PERSPECTIVE;
    app.last_step      = GetTime();

    app.world.rebuild(static_cast<int>(app.config.subdiv));
    app.world.restart(app.config, {0.0f, 0.0f, 9.0f});

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
    CloseWindow();
#endif
    return 0;
}
