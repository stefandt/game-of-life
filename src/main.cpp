#include "raylib.h"
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
    cam.position  = {0.0f, 0.0f, 9.0f};
    cam.target    = {0.0f, 0.0f, 0.0f};
    cam.up        = {0.0f, 1.0f, 0.0f};
    cam.fovy      = 45.0f;
    cam.projection = CAMERA_PERSPECTIVE;

    while (!WindowShouldClose()) {
        UpdateCamera(&cam, CAMERA_ORBITAL);

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
        DrawFPS(10, 40);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
