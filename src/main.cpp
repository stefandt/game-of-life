#include "raylib.h"
#include "raymath.h"
#include "hexsphere.h"

int main()
{
    InitWindow(1024, 768, "Hex Sphere");
    SetTargetFPS(60);

    const HexSphere sphere = HexSphere::create(3, 3.0f);

    int hex_count = 0;
    for (const auto& f : sphere.faces)
        if (!f.pentagon) ++hex_count;

    Camera3D cam  = {};
    cam.target    = {0.0f, 0.0f, 0.0f};
    cam.up        = {0.0f, 1.0f, 0.0f};
    cam.fovy      = 45.0f;
    cam.projection = CAMERA_PERSPECTIVE;

    float yaw      = 0.0f;    // горизонтальный угол, градусы
    float pitch    = 20.0f;   // вертикальный угол, градусы
    float distance = 9.0f;    // расстояние от центра

    while (!WindowShouldClose()) {
        // Левая кнопка мыши — вращение
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            Vector2 delta = GetMouseDelta();
            yaw   -= delta.x * 0.3f;
            pitch -= delta.y * 0.3f;
            pitch  = Clamp(pitch, -89.0f, 89.0f);
        }

        // Колёсико — зум
        float wheel = GetMouseWheelMove();
        if (wheel != 0.0f) {
            distance -= wheel * 0.5f;
            distance  = Clamp(distance, 4.0f, 20.0f);
        }

        // Позиция камеры из сферических координат
        float yr = yaw   * DEG2RAD;
        float pr = pitch * DEG2RAD;
        cam.position = {
            distance * cosf(pr) * sinf(yr),
            distance * sinf(pr),
            distance * cosf(pr) * cosf(yr)
        };

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

        DrawText(TextFormat("%d hexagons  12 pentagons", hex_count), 10, 10, 20, WHITE);
        DrawText("LMB drag: rotate   wheel: zoom", 10, 40, 18, GRAY);
        DrawFPS(10, 70);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
