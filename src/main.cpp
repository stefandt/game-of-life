#include "raylib.h"
#include "rlgl.h"
#include "raygui.h"
#include "game_config.h"
#include "game_world.h"
#include "cell_textures.h"
#include "cell_atlas.h"
#include "sim_panel.h"
#include "raymath.h"

int main()
{
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
    InitWindow(1500, 800, "Hex Sphere — Game of Life");
    SetTargetFPS(60);

    SimPanel   panel;
    GameConfig config;
    GameWorld  world;
    panel.init();

    world.rebuild(static_cast<int>(config.subdiv));
    world.restart(config, {0.0f, 0.0f, 9.0f});

    // ── Camera ────────────────────────────────────────────────────────────
    Camera3D cam   = {};
    cam.position   = {0.0f, 0.0f, 9.0f};
    cam.target     = {0.0f, 0.0f, 0.0f};
    cam.up         = {0.0f, 1.0f, 0.0f};
    cam.fovy       = 45.0f;
    cam.projection = CAMERA_PERSPECTIVE;

    float yaw = 0, pitch = 20, distance = 9;
    double last_step = GetTime();

    CellTextures tex;
    tex.load();

    CellAtlas atlas;
    atlas.load();
    world.build_render_mesh(atlas);  // pre-build once; update only on step()

    while (!WindowShouldClose()) {
        // ── Input ─────────────────────────────────────────────────────────
        if (IsKeyPressed(KEY_SPACE)) config.paused = !config.paused;
        if (IsKeyPressed(KEY_R)) {
            world.restart(config, cam);
            last_step = GetTime();
        }

        // Rules sync every frame — cheap (4 int assignments), ensures instant effect
        world.apply_rules(config);

        if (!config.paused && GetTime() - last_step >= 1.0 / config.speed) {
            world.step();
            world.update_render_mesh(atlas);  // update GPU buffers after state change
            last_step = GetTime();
        }

        // ── Camera ────────────────────────────────────────────────────────
        const bool mouse_in_panel = GetMouseX() > GetScreenWidth() - SimPanel::WIDTH;

        if (!panel.is_orbital && !mouse_in_panel) {
            if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
                Vector2 delta = GetMouseDelta();
                yaw   -= delta.x * 0.3f;
                pitch += delta.y * 0.3f;
                if (pitch > 89.0f)  { pitch = 180.0f - pitch; yaw += 180.0f; }
                if (pitch < -89.0f) { pitch = -180.0f - pitch; yaw += 180.0f; }
            }
            float wheel = GetMouseWheelMove();
            if (wheel != 0.0f)
                distance = Clamp(distance - wheel * 0.5f, 4.0f, 20.0f);
        }
        if (!panel.is_orbital) {
            float yr = yaw * DEG2RAD, pr = pitch * DEG2RAD;
            cam.position = { distance * cosf(pr) * sinf(yr),
                             distance * sinf(pr),
                             distance * cosf(pr) * cosf(yr) };
        } else {
            UpdateCamera(&cam, CAMERA_ORBITAL);
        }

        BeginDrawing();
        ClearBackground(BLACK);

        // ── 3D — viewport restricted to area left of panel ────────────────
        const int view_w = GetScreenWidth() - SimPanel::WIDTH;
        const int view_h = GetScreenHeight();
        rlViewport(0, 0, view_w, view_h);

        BeginMode3D(cam);

        // BeginMode3D uses full window for aspect ratio — override projection
        // so the sphere is centered in the 3D area, not the full window.
        rlMatrixMode(RL_PROJECTION);
        rlLoadIdentity();
        {
            const float aspect = (float)view_w / (float)view_h;
            const float top    = RL_CULL_DISTANCE_NEAR * tanf(cam.fovy * 0.5f * DEG2RAD);
            rlFrustum(-top * aspect, top * aspect, -top, top,
                      RL_CULL_DISTANCE_NEAR, RL_CULL_DISTANCE_FAR);
        }
        rlMatrixMode(RL_MODELVIEW);

        constexpr float EDGE_LIFT = 1.001f;
        const int nfaces = (int)world.sphere->faces.size();

        // ── Pass 1: fill cells ────────────────────────────────────────────
        rlEnableBackfaceCulling();

        if (panel.use_textures && world.render_mesh_ready) {
            // Pre-built Mesh — single DrawMesh() call, zero per-frame CPU work.
            // GPU buffers (UV + colors) updated only on step()/restart()/rebuild().
            DrawMesh(world.render_mesh, world.render_material, MatrixIdentity());

        } else {
            // Solid color from game field
            for (int fi = 0; fi < nfaces; ++fi) {
                const HexFace& face = world.sphere->faces[fi];
                const int      n    = (int)face.verts.size();
                const Vector3& c    = world.face_centers[fi];
                const Color    col  = world.field->cells[fi].color;
                for (int i = 0; i < n; ++i)
                    DrawTriangle3D(c,
                                   world.sphere->verts[face.verts[(i+1)%n]],
                                   world.sphere->verts[face.verts[i]], col);
            }
        }

        rlDisableBackfaceCulling();

        // ── Pass 2: edges — white hex, orange pent, front-facing only ─────
        // Edge color: light (white/orange) when textures are on so borders
        // are visible; dark when solid-color fill is used (subtle seam).
        auto draw_edges = [&](bool pent_only) {
            Color col;
            if (panel.use_textures)
                // col = pent_only ? ORANGE : RAYWHITE;
                col = pent_only ? Color{80, 40, 0, 200} : Color{0, 0, 0, 200};
            else
                col = pent_only ? Color{80, 40, 0, 200} : Color{0, 0, 0, 200};

            for (int fi = 0; fi < nfaces; ++fi) {
                const HexFace& face = world.sphere->faces[fi];
                if (face.pentagon != pent_only) continue;
                const Vector3& c = world.face_centers[fi];
                if (Vector3DotProduct(c, Vector3Subtract(c, cam.position)) >= 0.0f)
                    continue;
                const int n = (int)face.verts.size();
                for (int i = 0; i < n; ++i)
                    DrawLine3D(
                        Vector3Scale(world.sphere->verts[face.verts[i]], EDGE_LIFT),
                        Vector3Scale(world.sphere->verts[face.verts[(i+1)%n]], EDGE_LIFT),
                        col);
            }
        };

        if (!panel.use_textures) {
            rlEnableSmoothLines();
            draw_edges(false);
            draw_edges(true);
            rlDisableSmoothLines();
        } else {
            rlEnableSmoothLines();
            draw_edges(false);
            draw_edges(true);
            rlDisableSmoothLines();
        }
  

        EndMode3D();

        // Restore full viewport for 2D overlay and panel
        rlViewport(0, 0, GetScreenWidth(), GetScreenHeight());

        // ── Canvas overlay ────────────────────────────────────────────────
        DrawRectangle(0, 0, 240, 62, Fade(BLACK, 0.55f));
        DrawTextPro(GetFontDefault(),
            TextFormat("Generation: %d", world.generation), {10,10},{0,0},0,20,2,WHITE);
        DrawTextPro(GetFontDefault(),
            TextFormat("Alive: %d / %d", world.alive_count, world.total_cells()),
            {10,36},{0,0},0,20,2,LIME);
        DrawText(TextFormat("%d FPS", GetFPS()),
                 GetScreenWidth() - SimPanel::WIDTH - 70, 10, 20, LIME);

        // ── UI panel ──────────────────────────────────────────────────────
        panel.generation  = world.generation;
        panel.alive_count = world.alive_count;
        panel.total_cells = world.total_cells();
        panel.hex_count   = world.hex_count;
        panel.pent_count  = world.pent_count;

        panel.draw(config, cam, distance, pitch, yaw);

        if (panel.restart_requested) {
            world.restart(config, cam);
            world.update_render_mesh(atlas);
            last_step = GetTime();
        }
        if (panel.rebuild_requested) {
            world.rebuild(static_cast<int>(config.subdiv));
            world.restart(config, cam);
            world.build_render_mesh(atlas);   // geometry changed → full rebuild
            last_step = GetTime();
        }

        EndDrawing();
    }

    world.unload_render_mesh();
    tex.unload();
    atlas.unload();
    CloseWindow();
    return 0;
}
