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

    const HexSphere sphere = HexSphere::create(3, 3.0f);

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

    ViewMode mode  = ViewMode::MouseOrbit;

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
                pitch += delta.y * 0.3f;
                // Pole crossing: flip over instead of clamping
                if (pitch > 89.0f)  { pitch = 180.0f - pitch; yaw += 180.0f; }
                if (pitch < -89.0f) { pitch = -180.0f - pitch; yaw += 180.0f; }
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

        // Pass 1 — all hexagon edges (back edges visible — wireframe look)
        for (int fi = 0; fi < (int)sphere.faces.size(); ++fi) {
            if (sphere.faces[fi].pentagon) continue;
            const HexFace& face = sphere.faces[fi];
            const int n = (int)face.verts.size();
            for (int i = 0; i < n; ++i)
                DrawLine3D(sphere.verts[face.verts[i]],
                           sphere.verts[face.verts[(i + 1) % n]], RAYWHITE);
        }

        // Pass 2 — pentagon edges on top so shared edges stay orange
        for (int fi = 0; fi < (int)sphere.faces.size(); ++fi) {
            if (!sphere.faces[fi].pentagon) continue;
            const HexFace& face = sphere.faces[fi];
            const int n = (int)face.verts.size();
            for (int i = 0; i < n; ++i)
                DrawLine3D(sphere.verts[face.verts[i]],
                           sphere.verts[face.verts[(i + 1) % n]], ORANGE);
        }

        EndMode3D();

        // HUD
        DrawRectangle(0, 0, 420, 120, Fade(BLACK, 0.6f));

        hud(font, "Goldberg Polyhedron",                                      12, 10, 22, WHITE);
        hud(font, TextFormat("%d hexagons   %d pentagons", hex_count, pent_count),
            12, 36, 20, LIGHTGRAY);

        DrawLineEx({12, 61}, {408, 61}, 1, Fade(WHITE, 0.15f));

        if (mode == ViewMode::Orbital) {
            hud(font, "Auto-rotate  [Tab] Mouse Orbit", 12, 68, 16, DARKGRAY);
        } else {
            hud(font, "Mouse Orbit  [Tab] Auto-rotate", 12, 68, 16, YELLOW);
            hud(font, "[LMB] rotate   [Scroll] zoom",   12, 90, 16, DARKGRAY);
        }

        hud(font, TextFormat("%d FPS", GetFPS()),
            (float)GetScreenWidth() - 80.0f, 10.0f, 20.0f, LIME);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
