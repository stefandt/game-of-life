#include "game_world.h"
#include "raymath.h"

void GameWorld::rebuild(int subdiv)
{
    field.reset();
    sphere = std::make_unique<HexSphere>(HexSphere::create(subdiv, 3.0f));
    field  = std::make_unique<GameField>(*sphere);
    recompute_centers();
    recount_faces();
}

void GameWorld::recompute_centers()
{
    sphere_r = Vector3Length(sphere->verts[0]);
    face_centers.resize(sphere->faces.size());
    for (int fi = 0; fi < (int)sphere->faces.size(); ++fi) {
        const HexFace& face = sphere->faces[fi];
        const int n = (int)face.verts.size();
        Vector3 c = {0, 0, 0};
        for (int vi : face.verts) {
            c.x += sphere->verts[vi].x / n;
            c.y += sphere->verts[vi].y / n;
            c.z += sphere->verts[vi].z / n;
        }
        face_centers[fi] = Vector3Scale(Vector3Normalize(c), sphere_r);
    }
}

void GameWorld::recount_faces()
{
    hex_count = 0; pent_count = 0;
    for (const auto& f : sphere->faces) {
        if (f.pentagon) ++pent_count; else ++hex_count;
    }
}

void GameWorld::count_alive()
{
    alive_count = 0;
    for (const auto& c : field->cells) if (c.alive) ++alive_count;
}

void GameWorld::apply_rules(const GameConfig& cfg)
{
    switch (cfg.rules) {
        case Rules_B2_S23: field->rule_birth=2; field->rule_s_lo=2; field->rule_s_hi=3; break;
        case Rules_B3_S23: field->rule_birth=3; field->rule_s_lo=2; field->rule_s_hi=3; break;
        case Rules_B2_S34: field->rule_birth=2; field->rule_s_lo=3; field->rule_s_hi=4; break;
        default: break;
    }
}

int GameWorld::front_face(const Camera3D& cam) const
{
    const Vector3 dir = Vector3Normalize(cam.position);
    int best = 0; float best_d = -2.0f;
    for (int fi = 0; fi < (int)face_centers.size(); ++fi) {
        float d = Vector3DotProduct(Vector3Normalize(face_centers[fi]), dir);
        if (d > best_d) { best_d = d; best = fi; }
    }
    return best;
}

void GameWorld::restart(const GameConfig& cfg, const Camera3D& cam)
{
    apply_rules(cfg);
    field->seed(cfg.seed_count(), 42, front_face(cam));
    generation = 0;
    count_alive();
}

void GameWorld::step()
{
    field->step();
    ++generation;
    count_alive();
}
