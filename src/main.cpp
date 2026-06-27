#include "raylib.h"
#include "hexsphere.h"
#include "raymath.h"

enum class CameraMode { MouseOrbit, Orbital };

int main()
{
    InitWindow(1600, 1200, "Hex Sphere");
    SetTargetFPS(60);

    const HexSphere sphere = HexSphere::create(3, 3.0f);

    int hex_count = 0;
    for (const auto& f : sphere.faces)
        if (!f.pentagon) ++hex_count;

    Camera3D cam   = {};
    cam.target     = {0.0f, 0.0f, 0.0f};
    cam.up         = {0.0f, 1.0f, 0.0f};
    cam.fovy       = 45.0f;
    cam.projection = CAMERA_PERSPECTIVE;

    CameraMode mode = CameraMode::MouseOrbit;

    float yaw      = 0.0f;
    float pitch    = 20.0f;
    float distance = 9.0f;

    while (!WindowShouldClose()) {
        // Tab — переключение режима
        if (IsKeyPressed(KEY_TAB)) {
            mode = (mode == CameraMode::MouseOrbit)
                 ? CameraMode::Orbital
                 : CameraMode::MouseOrbit;
        }

        if (mode == CameraMode::MouseOrbit) {
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
        for (const auto& face : sphere.faces) {
            Color color = face.pentagon ? ORANGE : RAYWHITE;
            int n = (int)face.verts.size();
            for (int i = 0; i < n; ++i) {
                DrawLine3D(
                    sphere.verts[face.verts[i]],
                    sphere.verts[face.verts[(i + 1) % n]],
                    color
                );
            }
        }
        EndMode3D();

        const char* mode_str = (mode == CameraMode::MouseOrbit)
            ? "Mouse Orbit  (Tab to switch)"
            : "Auto Orbital  (Tab to switch)";

        DrawText(TextFormat("%d hexagons  12 pentagons", hex_count), 10, 10, 20, WHITE);
        DrawText(mode_str, 10, 40, 18, GRAY);
        if (mode == CameraMode::MouseOrbit)
            DrawText("LMB drag: rotate   wheel: zoom", 10, 62, 18, GRAY);
        DrawFPS(10, 90);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
