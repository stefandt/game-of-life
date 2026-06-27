#include "hexsphere.h"

#include <OpenMesh/Core/Mesh/PolyMesh_ArrayKernelT.hh>
#include <OpenMesh/Core/Mesh/TriMesh_ArrayKernelT.hh>
#include <OpenMesh/Tools/Subdivider/Uniform/LoopT.hh>

#include <array>
#include <cmath>
#include <vector>

using TriMesh = OpenMesh::TriMesh_ArrayKernelT<>;
using PolyMesh = OpenMesh::PolyMesh_ArrayKernelT<>;

namespace {

// Icosahedron — 12 vertices, 20 triangular faces.
// Vertex set: permutations of (0, ±1, ±φ), normalized to unit sphere.
// Face winding: CCW from outside (matches three.js IcosahedronGeometry).
TriMesh make_icosahedron()
{
    TriMesh m;
    const float t = (1.0f + sqrtf(5.0f)) / 2.0f;

    auto add = [&](float x, float y, float z) {
        float l = sqrtf(x * x + y * y + z * z);
        return m.add_vertex(TriMesh::Point(x / l, y / l, z / l));
    };

    std::array<TriMesh::VertexHandle, 12> v = {
        add(-1,  t,  0), add( 1,  t,  0),  // 0  1
        add(-1, -t,  0), add( 1, -t,  0),  // 2  3
        add( 0, -1,  t), add( 0,  1,  t),  // 4  5
        add( 0, -1, -t), add( 0,  1, -t),  // 6  7
        add( t,  0, -1), add( t,  0,  1),  // 8  9
        add(-t,  0, -1), add(-t,  0,  1),  // 10 11
    };

    auto f = [&](int a, int b, int c) { m.add_face(v[a], v[b], v[c]); };

    f(0,11,5);  f(0,5,1);   f(0,1,7);   f(0,7,10);  f(0,10,11);
    f(1,5,9);   f(5,11,4);  f(11,10,2); f(10,7,6);  f(7,1,8);
    f(3,9,4);   f(3,4,2);   f(3,2,6);   f(3,6,8);   f(3,8,9);
    f(4,9,5);   f(2,4,11);  f(6,2,10);  f(8,6,7);   f(9,8,1);

    return m;
}

// Dual mesh of a triangulated sphere → Goldberg polyhedron.
// Each primal face becomes a dual vertex (centroid projected to sphere).
// Each primal vertex becomes a dual face; vf_range gives CCW face order.
PolyMesh build_dual(const TriMesh& primal, float radius)
{
    PolyMesh dual;

    std::vector<PolyMesh::VertexHandle> fh_to_dvh(primal.n_faces());
    for (auto fh : primal.faces()) {
        TriMesh::Point c(0, 0, 0);
        for (auto vfh : primal.fv_range(fh))
            c += primal.point(vfh);
        c /= 3.0f;
        c = c.normalized() * radius;
        fh_to_dvh[fh.idx()] = dual.add_vertex(c);
    }

    for (auto vh : primal.vertices()) {
        std::vector<PolyMesh::VertexHandle> dv;
        for (auto fh : primal.vf_range(vh))
            dv.push_back(fh_to_dvh[fh.idx()]);
        if (dv.size() >= 3)
            dual.add_face(dv);
    }

    return dual;
}

}  // namespace

HexSphere HexSphere::create(int subdivisions, float radius)
{
    // 1. Base icosahedron
    TriMesh primal = make_icosahedron();

    // 2. Loop subdivision (each triangle → 4, vertices smoothed)
    OpenMesh::Subdivider::Uniform::LoopT<TriMesh> subdivider;
    subdivider.attach(primal);
    subdivider(subdivisions);
    subdivider.detach();

    // 3. Project all vertices onto the sphere
    for (auto vh : primal.vertices()) {
        auto p = primal.point(vh);
        primal.set_point(vh, p.normalized() * radius);
    }

    // 4. Build dual mesh (Goldberg polyhedron).
    //    Pentagon flag: original icosahedron vertices have valence 5 after
    //    subdivision; all other vertices have valence 6.
    //    Detect this from the primal before building the dual.
    std::vector<bool> is_pentagon(primal.n_vertices(), false);
    for (auto vh : primal.vertices())
        is_pentagon[vh.idx()] = (primal.valence(vh) == 5);

    const PolyMesh dual = build_dual(primal, radius);

    // 5. Extract into HexSphere
    HexSphere result;

    result.verts.resize(dual.n_vertices());
    for (auto vh : dual.vertices()) {
        auto p = dual.point(vh);
        result.verts[vh.idx()] = {p[0], p[1], p[2]};
    }

    // Dual face index == primal vertex index (same iteration order)
    result.faces.resize(dual.n_faces());
    int face_idx = 0;
    for (auto vh : primal.vertices()) {
        if (primal.vf_range(vh).begin() == primal.vf_range(vh).end()) continue;
        auto& face    = result.faces[face_idx];
        auto  dfh     = PolyMesh::FaceHandle(face_idx);
        face.pentagon = is_pentagon[vh.idx()];
        for (auto dvh : dual.fv_range(dfh))
            face.verts.push_back(dvh.idx());
        for (auto ffh : dual.ff_range(dfh))
            face.neighbors.push_back(ffh.idx());
        ++face_idx;
    }

    return result;
}
