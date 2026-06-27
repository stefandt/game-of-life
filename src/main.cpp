#include "raylib.h"
#include "hexsphere.h"
#include "raymath.h"

enum class ViewMode { Orbital, MouseOrbit };

static void hud(Font f, const char* s, float x, float y, float sz, Color c)
{
    DrawTextPro(f, s, {x, y}, {0, 0}, 0.0f, sz, sz * 0.16f, c);
}

int main()
{
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(1400, 800, "Hex Sphere — Goldberg Polyhedron");
    SetTargetFPS(60);

    Font font = GetFontDefault();
    SetTextureFilter(font.texture, TEXTURE_FILTER_POINT);
    
    // subdivisions: 1 → 42 faces, 2 → 162, 3 → 642, 4 → 2562
    const HexSphere sphere = HexSphere::create(4, 3.0f);

    int hex_count  = 0;
    int pent_count = 0;
    for (const auto& f : sphere.faces) {
        if (f.pentagon) ++pent_count;
        else            ++hex_count;
    }

    Camera3D cam   = {};
    cam.position   = {0.0f, 0.0f, 9.0f};
    cam.target     = {0.0f, 0.0f, 0.0f};
    cam.up         = {0.0f, 1.0f, 0.0f};
    cam.fovy       = 45.0f;
    cam.projection = CAMERA_PERSPECTIVE;

    ViewMode mode  = ViewMode::Orbital;

    float yaw      = 0.0f;
    float pitch    = 20.0f;
    float distance = 9.0f;

    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_TAB)) {
            if (mode == ViewMode::Orbital) {
                distance = Vector3Length(cam.position);
                pitch    = asinf(cam.position.y / distance) * RAD2DEG;
                yaw      = atan2f(cam.position.x, cam.position.z) * RAD2DEG;
                mode     = ViewMode::MouseOrbit;
            } else {
                mode = ViewMode::Orbital;
            }
        }

        if (mode == ViewMode::MouseOrbit) {
            if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
                Vector2 delta = GetMouseDelta();
                yaw   -= delta.x * 0.3f;
                pitch -= delta.y * 0.3f;
                pitch  = Clamp(pitch, -89.0f, 89.0f);
            }
            float wheel = GetMouseWheelMove();
            if (wheel != 0.0f) {
                distance -= wheel * 0.5f;
                distance  = Clamp(distance, 4.0f, 20.0f);
            }
            float yr = yaw   * DEG2RAD;
            float pr = pitch * DEG2RAD;
            cam.position = {
                distance * cosf(pr) * sinf(yr),
                distance * sinf(pr),
                distance * cosf(pr) * cosf(yr)
            };
        } else {
            UpdateCamera(&cam, CAMERA_ORBITAL);
        }

        BeginDrawing();
        ClearBackground(BLACK);

        BeginMode3D(cam);
        // Pass 1: hexagons
        for (const auto& face : sphere.faces) {
            if (face.pentagon) continue;
            int n = (int)face.verts.size();
            for (int i = 0; i < n; ++i)
                DrawLine3D(sphere.verts[face.verts[i]],
                           sphere.verts[face.verts[(i + 1) % n]], RAYWHITE);
        }
        // Pass 2: pentagons on top so shared edges stay orange
        for (const auto& face : sphere.faces) {
            if (!face.pentagon) continue;
            int n = (int)face.verts.size();
            for (int i = 0; i < n; ++i)
                DrawLine3D(sphere.verts[face.verts[i]],
                           sphere.verts[face.verts[(i + 1) % n]], ORANGE);
        }
        EndMode3D();

        // HUD
        DrawRectangle(0, 0, 420, 120, Fade(BLACK, 0.6f));

        hud(font, "Goldberg Polyhedron",                               12, 10, 22, WHITE);
        hud(font, TextFormat("%d hexagons   %d pentagons", hex_count, pent_count), 12, 36, 20, LIGHTGRAY);

        DrawLineEx({12, 61}, {408, 61}, 1, Fade(WHITE, 0.15f));

        if (mode == ViewMode::Orbital) {
            hud(font, "Auto-rotate",                  12, 68, 20, YELLOW);
            hud(font, "[Tab]  switch to Mouse Orbit", 12, 98, 20, DARKGRAY);
        } else {
            hud(font, "Mouse Orbit",                      12, 68, 20, YELLOW);
            hud(font, "[Tab]  switch to Auto-rotate",     12, 98, 20, DARKGRAY);
            hud(font, "[LMB] rotate  [Scroll] zoom",      20, 128, 20, DARKGRAY);
        }

        // FPS — top right, tracks window width on resize
        hud(font, TextFormat("%d FPS", GetFPS()),
            (float)GetScreenWidth() - 80.0f, 10.0f, 20.0f, LIME);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
