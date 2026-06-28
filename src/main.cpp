#include "raylib.h"
#include "rlgl.h"
#include "hexsphere.h"
#include "game_field.h"
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
    GameField field(sphere);

    // Face centers projected onto sphere — constant, computed once
    const float sphere_r = Vector3Length(sphere.verts[0]);
    std::vector<Vector3> face_centers(sphere.faces.size());
    for (int fi = 0; fi < (int)sphere.faces.size(); ++fi) {
        const HexFace& face = sphere.faces[fi];
        const int n = (int)face.verts.size();
        Vector3 c   = {0, 0, 0};
        for (int vi : face.verts) {
            c.x += sphere.verts[vi].x / n;
            c.y += sphere.verts[vi].y / n;
            c.z += sphere.verts[vi].z / n;
        }
        face_centers[fi] = Vector3Scale(Vector3Normalize(c), sphere_r);
    }

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

    ViewMode mode  = ViewMode::MouseOrbit;   // start in manual mode

    float yaw      = 0.0f;
    float pitch    = 20.0f;
    float distance = 9.0f;

    // Find face closest to camera start position (front of sphere)
    auto front_face = [&]() {
        const Vector3 cam_dir = Vector3Normalize(cam.position);
        int   best   = 0;
        float best_d = -2.0f;
        for (int fi = 0; fi < (int)face_centers.size(); ++fi) {
            float d = Vector3DotProduct(Vector3Normalize(face_centers[fi]), cam_dir);
            if (d > best_d) { best_d = d; best = fi; }
        }
        return best;
    };

    bool   paused    = false;
    double last_step = GetTime();
    field.seed(10, 42, front_face());

    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_SPACE)) paused = !paused;
        if (IsKeyPressed(KEY_R))     { field.seed(10, 42, front_face()); last_step = GetTime(); }

        if (!paused && GetTime() - last_step >= 1.0) {
            field.step();
            last_step = GetTime();
        }

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
                // Pole crossing: instead of clamping, flip over the pole
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

        // Pass 1 — filled cells, backface culled
        rlEnableBackfaceCulling();
        for (int fi = 0; fi < (int)sphere.faces.size(); ++fi) {
            const HexFace& face  = sphere.faces[fi];
            const int      n     = (int)face.verts.size();
            const Vector3& c     = face_centers[fi];
            for (int i = 0; i < n; ++i)
                DrawTriangle3D(c,
                               sphere.verts[face.verts[(i + 1) % n]],
                               sphere.verts[face.verts[i]],
                               field.cells[fi].color);
        }
        rlDisableBackfaceCulling();

        // Pass 2 — edges for front-facing cells only.
        // dot(face_normal, cam_to_face) < 0 means face points toward camera.
        // Edges lifted 0.4% above surface to avoid z-fighting with fill.
        constexpr float EDGE_LIFT = 1.004f;
        for (int fi = 0; fi < (int)sphere.faces.size(); ++fi) {
            const Vector3& c = face_centers[fi];
            // face_normal == normalize(c) == c / sphere_r
            // cam_to_face = c - cam.position
            if (Vector3DotProduct(c, Vector3Subtract(c, cam.position)) >= 0.0f)
                continue;  // back-facing — skip

            const HexFace& face = sphere.faces[fi];
            const int      n    = (int)face.verts.size();
            for (int i = 0; i < n; ++i)
                DrawLine3D(
                    Vector3Scale(sphere.verts[face.verts[i]],         EDGE_LIFT),
                    Vector3Scale(sphere.verts[face.verts[(i+1) % n]], EDGE_LIFT),
                    {0, 0, 0, 200});
        }

        EndMode3D();

        // HUD
        DrawRectangle(0, 0, 420, 120, Fade(BLACK, 0.6f));

        hud(font, "Goldberg Polyhedron",                               12, 10, 22, WHITE);
        hud(font, TextFormat("%d hexagons   %d pentagons", hex_count, pent_count), 12, 36, 20, LIGHTGRAY);

        DrawLineEx({12, 61}, {408, 61}, 1, Fade(WHITE, 0.15f));

        hud(font, paused ? "PAUSED  [Space] resume  [R] restart"
                         : "Running  [Space] pause  [R] restart",
            12, 68, 16, paused ? RED : GREEN);

        if (mode == ViewMode::Orbital) {
            hud(font, "Auto-rotate  [Tab] Mouse Orbit", 12, 92, 16, DARKGRAY);
        } else {
            hud(font, "Mouse Orbit  [Tab] Auto-rotate  [LMB] rotate  [Scroll] zoom",
                12, 92, 16, DARKGRAY);
        }

        // FPS — top right, tracks window width on resize
        hud(font, TextFormat("%d FPS", GetFPS()),
            (float)GetScreenWidth() - 80.0f, 10.0f, 20.0f, LIME);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
