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

// Precomputes per-face geometry that is constant for a given subdivision level
// and reused every frame:
//   face_centers[i] — centroid of face i projected onto the sphere surface.
//                     Used as the "pole" of the fan triangulation and for
//                     backface culling (dot product with camera direction).
//   face_tan1/tan2[i] — orthonormal tangent frame in the face's local plane.
//                       Used to compute radial UV coordinates when rendering
//                       with the texture atlas (project corner → angle → UV).
// Called once per rebuild(), not per frame.
void GameWorld::recompute_centers()
{
    sphere_r = Vector3Length(sphere->verts[0]);
    const int nf = (int)sphere->faces.size();
    face_centers.resize(nf);
    face_tan1.resize(nf);
    face_tan2.resize(nf);
    face_uvs.resize(nf);

    for (int fi = 0; fi < nf; ++fi) {
        const HexFace& face = sphere->faces[fi];
        const int n = (int)face.verts.size();

        Vector3 c = {0, 0, 0};
        for (int vi : face.verts) {
            c.x += sphere->verts[vi].x / n;
            c.y += sphere->verts[vi].y / n;
            c.z += sphere->verts[vi].z / n;
        }
        const Vector3 norm = Vector3Normalize(c);
        face_centers[fi] = Vector3Scale(norm, sphere_r);

        // Atlas convention: vertex 0 is at local (x=0, y=1) = top of cell.
        // UV formula: u = 0.5 + x/2,  v = 0.5 + y/2
        // So T2 = y-axis = toward vertex 0,
        //    T1 = x-axis = T2 × norm  (right-hand: x = y × z)
        const Vector3& v0 = sphere->verts[face.verts[0]];
        Vector3 to_v0 = { v0.x - face_centers[fi].x,
                           v0.y - face_centers[fi].y,
                           v0.z - face_centers[fi].z };
        const float dn = to_v0.x*norm.x + to_v0.y*norm.y + to_v0.z*norm.z;
        to_v0.x -= dn*norm.x;
        to_v0.y -= dn*norm.y;
        to_v0.z -= dn*norm.z;
        const Vector3 t2 = Vector3Normalize(to_v0);        // local y (toward v0 = atlas V1)
        const Vector3 t1 = Vector3CrossProduct(t2, norm); // local x
        face_tan1[fi] = t1;
        face_tan2[fi] = t2;

        // UV per vertex: project each vertex onto the tangent plane (T1/T2 frame),
        // UV from atlas formula: u_tile = 0.5 + x/2,  v_tile = 0.5 + y/2
        // where x = component along T1 (atlas x-axis),
        //       y = component along T2 (atlas y-axis = toward vertex 0)
        // Vertex 0 maps to (x=0, y=1) → tile UV (0.5, 1.0) matching atlas V1.
        // UV by vertex index: vertex j of n → fixed angle 2π·j/n from top.
        // Matches atlas convention exactly (V1 at top, CW order).
        // u = 0.5 + 0.5·sin(a),  v = 0.5 - 0.5·cos(a)
        face_uvs[fi].resize(n);
        const float step = 2.0f * PI / (float)n;
        for (int j = 0; j < n; ++j) {
            const float a = step * j;
            face_uvs[fi][j] = { 0.5f + 0.5f * sinf(a),
                                 0.5f - 0.5f * cosf(a) };
        }
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
        case Rules_B2_S23: field->rule_b_lo=2; field->rule_b_hi=2; field->rule_s_lo=2; field->rule_s_hi=3; break;
        case Rules_B3_S23: field->rule_b_lo=3; field->rule_b_hi=3; field->rule_s_lo=2; field->rule_s_hi=3; break;
        case Rules_B2_S34: field->rule_b_lo=2; field->rule_b_hi=2; field->rule_s_lo=3; field->rule_s_hi=4; break;
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
