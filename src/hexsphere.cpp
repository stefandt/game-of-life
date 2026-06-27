#include "hexsphere.h"
#include "par_shapes.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace {

// Centroid of triangle ti, projected onto unit sphere
Vector3 tri_centroid_on_sphere(const par_shapes_mesh* m, int ti)
{
    float x = 0, y = 0, z = 0;
    for (int j = 0; j < 3; ++j) {
        int vi = m->triangles[ti * 3 + j];
        x += m->points[vi * 3 + 0];
        y += m->points[vi * 3 + 1];
        z += m->points[vi * 3 + 2];
    }
    x /= 3.0f; y /= 3.0f; z /= 3.0f;
    float len = std::sqrt(x*x + y*y + z*z);
    return {x / len, y / len, z / len};
}

// Sort triangle indices around vertex vi by CCW angle in the tangent plane.
// Result is CCW when viewed from outside the sphere.
std::vector<int> sort_around_vertex(
    const par_shapes_mesh* m, int vi, const std::vector<int>& tris)
{
    float nx = m->points[vi*3+0];
    float ny = m->points[vi*3+1];
    float nz = m->points[vi*3+2];

    // Build an orthonormal tangent frame (T1, T2) perpendicular to N
    float ux = (std::abs(nx) < 0.9f) ? 1.0f : 0.0f;
    float uy = (std::abs(nx) < 0.9f) ? 0.0f : 1.0f;
    float uz = 0.0f;
    float d  = ux*nx + uy*ny + uz*nz;
    float t1x = ux - d*nx, t1y = uy - d*ny, t1z = uz - d*nz;
    float t1l = std::sqrt(t1x*t1x + t1y*t1y + t1z*t1z);
    t1x /= t1l; t1y /= t1l; t1z /= t1l;
    // T2 = N × T1  (CCW rotation axis when viewed from outside)
    float t2x = ny*t1z - nz*t1y;
    float t2y = nz*t1x - nx*t1z;
    float t2z = nx*t1y - ny*t1x;

    std::vector<std::pair<float, int>> angle_ti;
    angle_ti.reserve(tris.size());
    for (int ti : tris) {
        Vector3 c = tri_centroid_on_sphere(m, ti);
        // Vector from vertex to centroid, projected to tangent plane
        float dx = c.x - nx, dy = c.y - ny, dz = c.z - nz;
        float nd = dx*nx + dy*ny + dz*nz;
        float px = dx - nd*nx, py = dy - nd*ny, pz = dz - nd*nz;
        float angle = std::atan2(px*t2x + py*t2y + pz*t2z,
                                  px*t1x + py*t1y + pz*t1z);
        angle_ti.push_back({angle, ti});
    }
    std::sort(angle_ti.begin(), angle_ti.end());

    std::vector<int> result;
    result.reserve(tris.size());
    for (auto& [a, ti] : angle_ti) result.push_back(ti);
    return result;
}

} // namespace

HexSphere HexSphere::create(int subdivisions, float radius)
{
    par_shapes_mesh* geo = par_shapes_create_subdivided_sphere(subdivisions);

    // vertex → adjacent triangle indices
    std::vector<std::vector<int>> v2t(geo->npoints);
    for (int ti = 0; ti < geo->ntriangles; ++ti)
        for (int j = 0; j < 3; ++j)
            v2t[geo->triangles[ti*3 + j]].push_back(ti);

    HexSphere result;

    // Dual vertices = triangle centroids on sphere
    result.verts.resize(geo->ntriangles);
    for (int ti = 0; ti < geo->ntriangles; ++ti) {
        Vector3 c = tri_centroid_on_sphere(geo, ti);
        result.verts[ti] = {c.x * radius, c.y * radius, c.z * radius};
    }

    // Dual faces = one per primal vertex
    result.faces.reserve(geo->npoints);
    for (int vi = 0; vi < geo->npoints; ++vi) {
        const auto& tris = v2t[vi];
        if (tris.empty()) continue;

        HexFace face;
        face.verts    = sort_around_vertex(geo, vi, tris);
        face.pentagon = (face.verts.size() == 5);
        result.faces.push_back(std::move(face));
    }

    par_shapes_free_mesh(geo);
    return result;
}
