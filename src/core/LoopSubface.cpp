#include "LoopSubface.hpp"

#include <fstream>
#include <iostream>
#include <map>
#include <set>

#include <spdlog/spdlog.h>

#include "Timer.hpp"

#include "meshoptimizer/meshoptimizer.h"

namespace subface {

void LoopSubface::BuildTopology(const std::vector<glm::vec3>& positions, const std::vector<uint32_t>& indexes,
    std::vector<Vertex>& vertexes, std::vector<Face>& faces)
{
    size_t vertex_count = positions.size();
    size_t face_count = indexes.size() / 3;

    vertexes.resize(vertex_count);
    // Initialize vertexes' positions.
    for (int i = 0; i < vertex_count; ++i)
        vertexes[i].p = positions[i];

    faces.resize(face_count);
    // Initialize faces' vertexes and vertexes' `start_face`.
    for (int i = 0; i < face_count; ++i)
        for (int j = 0; j < 3; j++) {
            faces[i].v[j] = &vertexes[indexes[i * 3 + j]];
            // `start_face` of the same vertex may be updated multiple times.
            vertexes[indexes[i * 3 + j]].start_face = &faces[i];
        }

    // Compute faces' neighbors.
    // A local variable for temp usage.
    std::set<Edge> edges;
    for (Face& f : faces)
        for (int vi = 0; vi < 3; ++vi) {
            int v0 = vi, v1 = NEXT(vi);
            Edge e(f.v[v0], f.v[v1]);
            auto e_it = edges.find(e);
            if (e_it == edges.end()) {
                e.f = &f;
                e.id = vi;
                edges.insert(e);
            } else {
                e_it->f->neighbors[e_it->id] = &f;
                f.neighbors[vi] = e_it->f;
                edges.erase(e_it);
            }
        }

    // Update vertexes' `start_face`. Initialize vertexes' `boundary`, `valence` and `regular`.
    for (Vertex& v : vertexes) {
        v.ComputeStartFaceAndBoundary();
        v.ComputeValence();
        v.ComputeRegular();
    }
}

void LoopSubface::BuildTopology(const std::vector<glm::vec3>& positions, const std::vector<uint32_t>& indexes)
{
    origin_positions_ = positions;
    origin_indexes_ = indexes;

    BuildTopology(origin_positions_, origin_indexes_, vertexes_, faces_);
}

float LoopSubface::Beta(int valence)
{
    // Min valence for non-boundary vertexes is 3.
    if (valence == 3)
        // Larger than 3/(8*3)=3/24 to weigh less for the center vertex.
        return 3.f / 16.f;
    else
        return 3.f / (valence * 8.f);
}

float LoopSubface::LoopGamma(int valence)
{
    return 1.f / (valence + 3.f / (Beta(valence) * 8.f));
}

glm::vec3 LoopSubface::WeightOneRing(Vertex* v, float beta)
{
    int valence = v->valence;
    std::vector<const Vertex*> ring = v->OneRing();
    glm::vec3 p = (1 - valence * beta) * v->p;
    for (int i = 0; i < valence; ++i)
        p += beta * ring[i]->p;
    return p;
}

glm::vec3 LoopSubface::WeightBoundary(Vertex* v, float beta)
{
    std::vector<const Vertex*> boundary_neighbors = v->BoundaryNeighbors();
    glm::vec3 p = (1 - beta * 2.f) * v->p + (boundary_neighbors[0]->p + boundary_neighbors[1]->p) * beta;
    return p;
}

void LoopSubface::ComputeNormalsAndPositions(const std::vector<Vertex*>& vertexes, const std::vector<Face*>& faces)
{
    // Compute vertexes' smooth normals.
    std::vector<glm::vec3> smooth_normals(vertexes.size());
    for (int vi = 0; vi < vertexes.size(); ++vi) {
        const Vertex* v = vertexes[vi];

        glm::vec3 S(0, 0, 0), T(0, 0, 0);
        size_t valence = v->valence;
        std::vector<const Vertex*> ring = v->OneRing();
        if (!v->boundary) {
            for (int i = 0; i < valence; ++i) {
                T += std::cos(2.f * PI * i / valence) * ring[i]->p;
                S += std::sin(2.f * PI * i / valence) * ring[i]->p;
            }
        } else {
            S = ring[valence - 1]->p - ring[0]->p;
            if (valence == 2)
                T = -v->p * 2.f + ring[0]->p + ring[1]->p;
            else if (valence == 3)
                T = -v->p + ring[1]->p;
            else if (valence == 4)
                T = -v->p * 2.f - ring[0]->p + ring[1]->p * 2.f + ring[2]->p * 2.f - ring[3]->p;
            else {
                float theta = PI / float(valence - 1);
                T = std::sin(theta) * (ring[0]->p + ring[valence - 1]->p);
                for (int i = 1; i < valence - 1; ++i) {
                    float weight = (std::cos(theta) * 2.f - 2.f) * std::sin(theta * i);
                    T += ring[i]->p * weight;
                }
                T = -T;
            }
        }
        smooth_normals[vi] = glm::normalize(glm::cross(S, T));
    }

    // Vertex indexes.
    std::map<const Vertex*, int> vertex2index;
    for (size_t i = 0; i < vertexes.size(); ++i)
        vertex2index[vertexes[i]] = static_cast<int>(i);

    indexed_positions_.resize(vertexes.size());
    for (size_t i = 0; i < vertexes.size(); ++i)
        indexed_positions_[i] = vertexes[i]->p;

    vertex_indexes_.resize(faces.size() * 3);
    smooth_normal_indexes_.resize(faces.size() * 3);
    flat_normal_indexes_.resize(faces.size() * 3);
    for (size_t i = 0; i < faces.size(); ++i)
        for (int j = 0; j < 3; ++j) {
            vertex_indexes_[i * 3 + j] = vertex2index[faces[i]->v[j]];
            smooth_normal_indexes_[i * 3 + j] = vertex2index[faces[i]->v[j]];
        }
    indexed_flat_normals_.resize(faces.size());

    // Unindexd vertexes and normals.
    unindexed_positions_.resize(faces.size() * 3);
    unindexed_smooth_normals_.resize(faces.size() * 3);
    unindexed_flat_normals_.resize(faces.size() * 3);
    for (size_t i = 0; i < faces.size(); i++) {
        glm::vec3 normal_flat = glm::normalize(glm::cross(
            faces[i]->v[1]->p - faces[i]->v[0]->p,
            faces[i]->v[2]->p - faces[i]->v[1]->p));
        for (int j = 0; j < 3; j++) {
            unindexed_positions_[i * 3 + j] = faces[i]->v[j]->p;
            unindexed_smooth_normals_[i * 3 + j] = smooth_normals[vertex2index[faces[i]->v[j]]];
            unindexed_flat_normals_[i * 3 + j] = normal_flat;
            flat_normal_indexes_[i * 3 + j] = static_cast<int>(i);
        }
        indexed_flat_normals_[i] = normal_flat;
    }

    indexed_smooth_normals_ = std::move(smooth_normals);
}

void LoopSubface::Subdivide(int level, bool flat, bool compute_limit)
{
    std::string func_name = fmt::format("LoopSubface::Subdivide(level={}, flat={}, compute_limit={})", level, flat, compute_limit);
    Timer timer(func_name);

    level_ = level;

    // Ptrs of base vertexes and faces for the current level.
    std::vector<Vertex*> vertexes_base(vertexes_.size());
    std::vector<Face*> faces_base(faces_.size());
    for (size_t i = 0; i < vertexes_.size(); i++)
        vertexes_base[i] = &vertexes_[i];
    for (size_t i = 0; i < faces_.size(); i++)
        faces_base[i] = &faces_[i];

    MemoryPool mp;
    for (int l = 0; l < level; ++l) {
        std::vector<Vertex*> vertexes_new;
        std::vector<Face*> faces_new(faces_base.size() * 4);

        for (auto& v : vertexes_base) {
            vertexes_new.emplace_back(mp.New<Vertex>());
            v->child = vertexes_new.back();
            v->child->regular = v->regular;
            v->child->boundary = v->boundary;
            v->child->valence = v->valence;
        }
        // Create new faces here for the convinience of updating new vertexes' `start_face` ptr.
        // After updating new vertexes, new faces are updated.
        for (size_t fi = 0; fi < faces_base.size(); ++fi)
            for (int ci = 0; ci < 4; ++ci)
                faces_base[fi]->children[ci] = faces_new[fi * 4 + ci] = mp.New<Face>();

        // Update new base vertexes.
        for (auto& v : vertexes_base)
            if (flat) {
                v->child->p = v->p;
            } else {
                if (!v->boundary) {
                    //   \ /   //
                    // -- * -- //
                    //   / \   //
                    // (1-6*1/16) for the center vertex, (1/16) for each of the 6 neighbor vertexes.
                    if (v->regular)
                        v->child->p = WeightOneRing(v, 1.f / 16.f);
                    // (1-Valence*Beta) for the center vertex, (Beta) for each of the Valence neighbor vertexes.
                    else
                        v->child->p = WeightOneRing(v, Beta(v->valence));
                } else {
                    //      0 ... 0      //
                    //       \.../       //
                    // 1/8 -- 3/4 -- 1/8 //
                    // Only the boundary vertexes are used.
                    v->child->p = WeightBoundary(v, 1.f / 8.f);
                }
            }

        // Add a new sub-vertex on each edge.
        std::map<Edge, Vertex*> edge2vertex;
        for (auto& f : faces_base) {
            for (int vi = 0; vi < 3; ++vi) {
                Edge e(f->v[vi], f->v[NEXT(vi)]);
                Vertex* v = edge2vertex[e];
                if (!v) {
                    vertexes_new.emplace_back(mp.New<Vertex>());
                    v = vertexes_new.back();
                    // All new sub-vertexes are regular no matter they are on boundary edges or not.
                    // That's why we have this concept of "regular" and special processing for regular vertexes.
                    v->regular = true;
                    v->boundary = (f->neighbors[vi] == nullptr);
                    v->valence = v->boundary ? 4 : 6;
                    v->start_face = f->children[vi];

                    if (flat || v->boundary) {
                        v->p = 0.5f * e.v[0]->p;
                        v->p += 0.5f * e.v[1]->p;
                    } else {
                        //     *   //
                        //    / \  //
                        //   *-O-* //
                        //    \ /  //
                        //     *   //
                        //
                        //    1/8    //
                        //    / \    //
                        // 3/8 - 3/8 //
                        //    \ /    //
                        //    1/8    //
                        v->p = 3.f / 8.f * e.v[0]->p;
                        v->p += 3.f / 8.f * e.v[1]->p;
                        v->p += 1.f / 8.f
                            * f->OtherVertex(e.v[0], e.v[1])->p;
                        v->p += 1.f / 8.f
                            * f->neighbors[vi]->OtherVertex(e.v[0], e.v[1])->p;
                    }
                    edge2vertex[e] = v;
                }
            }
        }

        // Update `start_face` of new base vertexes as new sub-faces.
        for (auto& v : vertexes_base) {
            int vi = v->start_face->VertexId(v);
            v->child->start_face = v->start_face->children[vi];
        }

        // Update new sub-faces.
        for (auto& f : faces_base) {
            for (int ci = 0; ci < 3; ++ci) {
                // Update new sub-faces' neighbors.
                {
                    //     1
                    //    /1\
                    //   0 - 1
                    //  /0\3/2\
                    // 0 - 2 - 2
                    // `neighbors[i]` is the face sharing the edge of v[i] and v[NEXT(i)].

                    f->children[3]->neighbors[ci] = f->children[NEXT(ci)];
                    f->children[ci]->neighbors[NEXT(ci)] = f->children[3];

                    const Face* fn = f->neighbors[ci];
                    fn ? f->children[ci]->neighbors[ci] = fn->children[fn->VertexId(f->v[ci])] : 0;
                    fn = f->neighbors[PREV(ci)];
                    fn ? f->children[ci]->neighbors[PREV(ci)] = fn->children[fn->VertexId(f->v[ci])] : 0;
                    // `f->children[ci]->neighbors[k]` is default as `nullptr`. No need to assign in `else`.
                }
                // Update new sub-faces' vertexes.
                {
                    f->children[ci]->v[ci] = f->v[ci]->child;

                    // 3 new sub-faces share the the same new sub-vertex.
                    Vertex* vertex = edge2vertex[Edge(f->v[ci], f->v[NEXT(ci)])];
                    f->children[ci]->v[NEXT(ci)] = vertex;
                    f->children[NEXT(ci)]->v[ci] = vertex;
                    f->children[3]->v[ci] = vertex;
                }
            }
        }

        // All updates done. Replace the base with the new for further subdivisions.
        vertexes_base = std::move(vertexes_new);
        faces_base = std::move(faces_new);
    }

    if (!flat && level && compute_limit) {
        std::vector<glm::vec3> limit(vertexes_base.size());
        for (size_t i = 0; i < vertexes_base.size(); ++i)
            if (vertexes_base[i]->boundary)
                limit[i] = WeightBoundary(vertexes_base[i], 1.f / 5.f);
            else
                limit[i] = WeightOneRing(vertexes_base[i], LoopGamma(vertexes_base[i]->valence));
        for (size_t i = 0; i < vertexes_base.size(); ++i)
            vertexes_base[i]->p = limit[i];
    }

    ComputeNormalsAndPositions(vertexes_base, faces_base);

    spdlog::info("{}: {} triangles, {} vertexes", func_name, faces_base.size(), vertexes_base.size());
}

void LoopSubface::Tessellate3(int level)
{
    std::string func_name = fmt::format("LoopSubface::Tessellate3(level={})", level);
    Timer timer(func_name);

    level_ = level;

    // Ptrs of base vertexes and faces for the current level.
    std::vector<Vertex*> vertexes_base(vertexes_.size());
    std::vector<Face*> faces_base(faces_.size());
    for (size_t i = 0; i < vertexes_.size(); i++)
        vertexes_base[i] = &vertexes_[i];
    for (size_t i = 0; i < faces_.size(); i++)
        faces_base[i] = &faces_[i];

    MemoryPool mp;
    for (int l = 0; l < level; ++l) {
        std::vector<Vertex*> vertexes_new;
        std::vector<Face*> faces_new(faces_base.size() * 3);

        for (auto& v : vertexes_base) {
            vertexes_new.emplace_back(mp.New<Vertex>());
            v->child = vertexes_new.back();
            // `regular` is useless for tessellation.
            v->child->p = v->p;
            v->child->boundary = v->boundary;
            v->child->valence = v->valence * 2 - (v->boundary ? 1 : 0);
        }
        // Create new faces here for the convinience of updating new vertexes' `start_face` ptr.
        // After updating new vertexes, new faces are updated.
        for (size_t fi = 0; fi < faces_base.size(); ++fi)
            for (int ci = 0; ci < 3; ++ci)
                faces_base[fi]->children[ci] = faces_new[fi * 3 + ci] = mp.New<Face>();

        // Add a new sub-vertex on each face.
        for (size_t fi = 0; fi < faces_base.size(); ++fi) {
            Face* f = faces_base[fi];

            vertexes_new.emplace_back(mp.New<Vertex>());
            Vertex* v = vertexes_new.back();
            v->boundary = false;
            v->valence = 3;
            v->start_face = f->children[0];

            v->p = (f->v[0]->p + f->v[1]->p + f->v[2]->p) / 3.f;
        }

        // Update `start_face` of new base vertexes as new sub-faces.
        for (auto& v : vertexes_base) {
            int vi = v->start_face->VertexId(v);
            if (v->start_face->neighbors[vi] == nullptr)
                v->child->start_face = v->start_face->children[vi];
            else
                v->child->start_face = v->start_face->children[PREV(vi)];
        }

        // Update new sub-faces.
        for (size_t fi = 0; fi < faces_base.size(); ++fi) {
            Face* f = faces_base[fi];
            for (int ci = 0; ci < 3; ++ci) {
                // Update new sub-faces' neighbors.
                {
                    // `neighbors[i]` is the face sharing the edge of v[i] and v[NEXT(i)].
                    f->children[ci]->neighbors[PREV(ci)] = f->children[PREV(ci)];
                    f->children[ci]->neighbors[NEXT(ci)] = f->children[NEXT(ci)];

                    const Face* fn = f->neighbors[ci];
                    if (fn) {
                        int fn_ci = fn->VertexId(f->v[ci]);
                        if (f->OppositeNeighbor(ci))
                            f->children[ci]->neighbors[ci] = fn->children[fn_ci];
                        else
                            f->children[ci]->neighbors[ci] = fn->children[PREV(fn_ci)];
                    }
                    // `f->children[ci]->neighbors[k]` is default as `nullptr`. No need to assign in `else`.
                }
                // Update new sub-faces' vertexes.
                {
                    f->children[ci]->v[ci] = f->v[ci]->child;
                    f->children[ci]->v[NEXT(ci)] = f->v[NEXT(ci)]->child;
                    f->children[ci]->v[PREV(ci)] = vertexes_new[vertexes_base.size() + fi];
                }
            }
        }

        // All updates done. Replace the base with the new for further subdivisions.
        vertexes_base = std::move(vertexes_new);
        faces_base = std::move(faces_new);
    }

    ComputeNormalsAndPositions(vertexes_base, faces_base);

    spdlog::info("{}: {} triangles, {} vertexes", func_name, faces_base.size(), vertexes_base.size());
}

void LoopSubface::Tessellate4(int level)
{
    std::string func_name = fmt::format("LoopSubface::Tessellate4(level={})", level);
    Timer timer(func_name);

    level_ = level;

    // Ptrs of base vertexes and faces for the current level.
    std::vector<Vertex*> vertexes_base(vertexes_.size());
    std::vector<Face*> faces_base(faces_.size());
    for (size_t i = 0; i < vertexes_.size(); i++)
        vertexes_base[i] = &vertexes_[i];
    for (size_t i = 0; i < faces_.size(); i++)
        faces_base[i] = &faces_[i];

    MemoryPool mp;
    for (int l = 0; l < level; ++l) {
        std::vector<Vertex*> vertexes_new;
        std::vector<Face*> faces_new(faces_base.size() * 4);

        for (auto& v : vertexes_base) {
            vertexes_new.emplace_back(mp.New<Vertex>());
            v->child = vertexes_new.back();
            v->child->p = v->p;
            v->child->regular = v->regular;
            v->child->boundary = v->boundary;
            v->child->valence = v->valence;
        }
        // Create new faces here for the convinience of updating new vertexes' `start_face` ptr.
        // After updating new vertexes, new faces are updated.
        for (size_t fi = 0; fi < faces_base.size(); ++fi)
            for (int ci = 0; ci < 4; ++ci)
                faces_base[fi]->children[ci] = faces_new[fi * 4 + ci] = mp.New<Face>();

        // Add a new sub-vertex on each edge.
        std::map<Edge, Vertex*> edge2vertex;
        for (auto& f : faces_base) {
            for (int vi = 0; vi < 3; ++vi) {
                Edge e(f->v[vi], f->v[NEXT(vi)]);
                Vertex* v = edge2vertex[e];
                if (!v) {
                    vertexes_new.emplace_back(mp.New<Vertex>());
                    v = vertexes_new.back();
                    // All new sub-vertexes are regular no matter they are on boundary edges or not.
                    // That's why we have this concept of "regular" and special processing for regular vertexes.
                    v->regular = true;
                    v->boundary = (f->neighbors[vi] == nullptr);
                    v->valence = v->boundary ? 4 : 6;
                    v->start_face = f->children[vi];

                    v->p = 0.5f * e.v[0]->p;
                    v->p += 0.5f * e.v[1]->p;
                    edge2vertex[e] = v;
                }
            }
        }

        // Update `start_face` of new base vertexes as new sub-faces.
        for (auto& v : vertexes_base) {
            int vi = v->start_face->VertexId(v);
            v->child->start_face = v->start_face->children[vi];
        }

        // Update new sub-faces.
        for (auto& f : faces_base) {
            for (int ci = 0; ci < 3; ++ci) {
                // Update new sub-faces' neighbors.
                {
                    //     1
                    //    /1\
                    //   0 - 1
                    //  /0\3/2\
                    // 0 - 2 - 2
                    // `neighbors[i]` is the face sharing the edge of v[i] and v[NEXT(i)].

                    f->children[3]->neighbors[ci] = f->children[NEXT(ci)];
                    f->children[ci]->neighbors[NEXT(ci)] = f->children[3];

                    const Face* fn = f->neighbors[ci];
                    fn ? f->children[ci]->neighbors[ci] = fn->children[fn->VertexId(f->v[ci])] : 0;
                    fn = f->neighbors[PREV(ci)];
                    fn ? f->children[ci]->neighbors[PREV(ci)] = fn->children[fn->VertexId(f->v[ci])] : 0;
                    // `f->children[ci]->neighbors[k]` is default as `nullptr`. No need to assign in `else`.
                }
                // Update new sub-faces' vertexes.
                {
                    f->children[ci]->v[ci] = f->v[ci]->child;

                    // 3 new sub-faces share the the same new sub-vertex.
                    Vertex* vertex = edge2vertex[Edge(f->v[ci], f->v[NEXT(ci)])];
                    f->children[ci]->v[NEXT(ci)] = vertex;
                    f->children[NEXT(ci)]->v[ci] = vertex;
                    f->children[3]->v[ci] = vertex;
                }
            }
        }

        // All updates done. Replace the base with the new for further subdivisions.
        vertexes_base = std::move(vertexes_new);
        faces_base = std::move(faces_new);
    }

    ComputeNormalsAndPositions(vertexes_base, faces_base);

    spdlog::info("{}: {} triangles, {} vertexes", func_name, faces_base.size(), vertexes_base.size());
}

void LoopSubface::Tessellate4_1(int level)
{
    std::string func_name = fmt::format("LoopSubface::Tessellate4_1(level={})", level);
    Timer timer(func_name);

    level_ = level;

    // Ptrs of base vertexes and faces for the current level.
    std::vector<Vertex*> vertexes_base(vertexes_.size());
    std::vector<Face*> faces_base(faces_.size());
    for (size_t i = 0; i < vertexes_.size(); i++)
        vertexes_base[i] = &vertexes_[i];
    for (size_t i = 0; i < faces_.size(); i++)
        faces_base[i] = &faces_[i];

    MemoryPool mp;
    for (int l = 0; l < level; ++l) {
        std::vector<Vertex*> vertexes_new;
        std::vector<Face*> faces_new(faces_base.size() * 4);

        // Shift vertexes and neighbors so that the new divergency vertex (e.g. new vertex 2 on edge 0-2 in the following diagram) is on the longest edges.
        //     1
        //    /|\
        //   01|21
        //  /0\|/3\
        // 0 - 2 - 2
        // The following tessellation code is based on the above tessellation pattern.
        // So given `longest_edge_id`, vertexes and neighbors need to shift by `longest_edge_id + 3 - 2`, or `longest_edge_id + 1`.
        //     1                            1
        //    /|\   longest_edge_id == 2   /|\
        //   01|21  ------------------->  01|21
        //  /0\|/3\       Shift(3)       /0\|/3\
        // 0 - 2 - 2     (no change)    0 - 2 - 2
        //
        //     1                            0
        //    /|\   longest_edge_id == 1   /|\
        //   01|21  ------------------->  21|20
        //  /0\|/3\       Shift(2)       /0\|/3\
        // 0 - 2 - 2                    2 - 1 - 1
        //
        //     1                            2
        //    /|\   longest_edge_id == 0   /|\
        //   01|21  ------------------->  11|22
        //  /0\|/3\       Shift(1)       /0\|/3\
        // 0 - 2 - 2                    1 - 0 - 0
        for (auto& f : faces_base) {
            float edge_lengths[3] {};
            for (int i = 0; i < 3; ++i)
                edge_lengths[i] = glm::distance(f->v[i]->p, f->v[NEXT(i)]->p);
            int longest_edge_id = static_cast<int>(std::max_element(edge_lengths, edge_lengths + 3) - edge_lengths);
            f->Shift(longest_edge_id + 1);
        }

        for (auto& v : vertexes_base) {
            vertexes_new.emplace_back(mp.New<Vertex>());
            v->child = vertexes_new.back();
            // `regular` is useless for tessellation.
            v->child->p = v->p;
            v->child->boundary = v->boundary;
            int vi_1_count = 0;
            v->TraverseFaces([&](const Face* f) {
                // Only vertex 1 gets 1 more valence.
                if (f->VertexId(v) == 1)
                    vi_1_count++;
            });
            v->child->valence = v->valence + vi_1_count;
        }
        // Create new faces here for the convinience of updating new vertexes' `start_face` ptr.
        // After updating new vertexes, new faces are updated.
        for (size_t fi = 0; fi < faces_base.size(); ++fi)
            for (int ci = 0; ci < 4; ++ci)
                faces_base[fi]->children[ci] = faces_new[fi * 4 + ci] = mp.New<Face>();

        // Use this `Edge` struct containing both neighbor triangles to perfectly fix the issue caused by the edges shared by more than 2 triangels.
        // Before, on the edges shared by k>2 triangles, only 1 vertex are created, for which the valence is confusing and causes issues.
        // Now (k+1)/2 vertexes are created on such edges. Each vertex is shared by at most 2 triangles.
        // An imperfect workaround is that, do nothing if `edge2vertex` already has the edge. This causes smaller valence for vertexes with ID 2.
        // So in `Vertex::OneRing()`, the lambda should return directly if `i >= valence` to prevent "vecter subscript out of range".
        // With smaller valence, hence wrong "OneRing", wrong smooth normals will be computed. But it doesn't matter for tessellation.
        struct Edge {
            const Vertex* v[2];
            const Face* f[2];

            Edge(const Vertex* v0 = nullptr, const Vertex* v1 = nullptr, const Face* f0 = nullptr, const Face* f1 = nullptr)
            {
                v[0] = std::min(v0, v1);
                v[1] = std::max(v0, v1);
                f[0] = std::min(f0, f1);
                f[1] = std::max(f0, f1);
            }

            bool operator<(const Edge& e) const
            {
                const std::array<void*, 4>& a = *reinterpret_cast<const std::array<void*, 4>*>(this);
                const std::array<void*, 4>& b = *reinterpret_cast<const std::array<void*, 4>*>(&e);
                return a < b;
            }
        };

        // Add a new sub-vertex on each edge.
        std::map<Edge, Vertex*> edge2vertex;
        for (auto& f : faces_base) {
            for (int vi = 0; vi < 3; ++vi) {
                Edge e(f->v[vi], f->v[NEXT(vi)], f, f->neighbors[vi]);
                Vertex* v = edge2vertex[e];
                if (!v) {
                    vertexes_new.emplace_back(mp.New<Vertex>());
                    v = vertexes_new.back();
                    v->boundary = (f->neighbors[vi] == nullptr);
                    v->valence = (v->boundary ? 3 : 4) + (vi == 2 ? 2 : 0);
                    v->start_face = f->children[vi == 0 ? 0 : vi + 1];

                    v->p = 0.5f * e.v[0]->p;
                    v->p += 0.5f * e.v[1]->p;
                    edge2vertex[e] = v;
                } else {
                    v->valence += (vi == 2 ? 2 : 0);
                }
            }
        }

        // Update `start_face` of new base vertexes as new sub-faces.
        for (auto& v : vertexes_base) {
            int vi = v->start_face->VertexId(v);
            v->child->start_face = v->start_face->children[vi == 2 ? 3 : vi];
        }

        // Update new sub-faces.
        for (auto& f : faces_base) {
            // Update new sub-faces' neighbors
            {
                //     1
                //    /|\
                //   01|21
                //  /0\|/3\
                // 0 - 2 - 2
                // `neighbors[i]` is the face sharing the edge of v[i] and v[NEXT(i)].

                const Face* f0 = f->neighbors[0];
                const Face* f1 = f->neighbors[1];
                const Face* f2 = f->neighbors[2];

                // Consider neighbor triangles with opposite normals.
                bool o0 = f->OppositeNeighbor(0);
                bool o1 = f->OppositeNeighbor(1);
                bool o2 = f->OppositeNeighbor(2);

                auto vertex_id_to_child_id_pre = [](int i) {
                    switch (i) {
                    case 0:
                        return 0;
                    case 1:
                        return 1;
                    case 2:
                        return 3;
                    default:
                        assert(false);
                        return 0;
                    }
                };
                auto vertex_id_to_child_id_nxt = [](int i) {
                    switch (i) {
                    case 0:
                        return 0;
                    case 1:
                        return 2;
                    case 2:
                        return 3;
                    default:
                        assert(false);
                        return 0;
                    }
                };

                int f0_v0_id = f0 ? f0->VertexId(f->v[0]) : -1;
                int f0_v1_id = f0 ? f0->VertexId(f->v[1]) : -1;
                int f1_v1_id = f1 ? f1->VertexId(f->v[1]) : -1;
                int f1_v2_id = f1 ? f1->VertexId(f->v[2]) : -1;
                int f2_v2_id = f2 ? f2->VertexId(f->v[2]) : -1;
                int f2_v0_id = f2 ? f2->VertexId(f->v[0]) : -1;

                f0 ? f->children[0]->neighbors[0] = f0->children[o0 ? vertex_id_to_child_id_nxt(f0_v0_id) : vertex_id_to_child_id_pre(f0_v0_id)] : 0;
                f->children[0]->neighbors[1] = f->children[1];
                f2 ? f->children[0]->neighbors[2] = f2->children[o2 ? vertex_id_to_child_id_pre(f2_v0_id) : vertex_id_to_child_id_nxt(f2_v0_id)] : 0;

                f0 ? f->children[1]->neighbors[0] = f0->children[o0 ? vertex_id_to_child_id_pre(f0_v1_id) : vertex_id_to_child_id_nxt(f0_v1_id)] : 0;
                f->children[1]->neighbors[1] = f->children[2];
                f->children[1]->neighbors[2] = f->children[0];

                f->children[2]->neighbors[0] = f->children[1];
                f1 ? f->children[2]->neighbors[1] = f1->children[o1 ? vertex_id_to_child_id_nxt(f1_v1_id) : vertex_id_to_child_id_pre(f1_v1_id)] : 0;
                f->children[2]->neighbors[2] = f->children[3];

                f->children[3]->neighbors[0] = f->children[2];
                f1 ? f->children[3]->neighbors[1] = f1->children[o1 ? vertex_id_to_child_id_pre(f1_v2_id) : vertex_id_to_child_id_nxt(f1_v2_id)] : 0;
                f2 ? f->children[3]->neighbors[2] = f2->children[o2 ? vertex_id_to_child_id_nxt(f2_v2_id) : vertex_id_to_child_id_pre(f2_v2_id)] : 0;

                // `f->children[ci]->neighbors[k]` is default as `nullptr`. No need to assign in `else`.
            }
            // Update new sub-faces' vertexes.
            {
                Edge e[3];
                for (int i = 0; i < 3; ++i)
                    e[i] = { f->v[i], f->v[NEXT(i)], f, f->neighbors[i] };

                f->children[0]->v[0] = f->v[0]->child;
                f->children[0]->v[1] = edge2vertex[e[0]];
                f->children[0]->v[2] = edge2vertex[e[2]];

                f->children[1]->v[0] = edge2vertex[e[0]];
                f->children[1]->v[1] = f->v[1]->child;
                f->children[1]->v[2] = edge2vertex[e[2]];

                f->children[2]->v[0] = edge2vertex[e[2]];
                f->children[2]->v[1] = f->v[1]->child;
                f->children[2]->v[2] = edge2vertex[e[1]];

                f->children[3]->v[0] = edge2vertex[e[2]];
                f->children[3]->v[1] = edge2vertex[e[1]];
                f->children[3]->v[2] = f->v[2]->child;
            }
        }

        // All updates done. Replace the base with the new for further subdivisions.
        vertexes_base = std::move(vertexes_new);
        faces_base = std::move(faces_new);
    }

    ComputeNormalsAndPositions(vertexes_base, faces_base);

    spdlog::info("{}: {} triangles, {} vertexes", func_name, faces_base.size(), vertexes_base.size());
}

template <typename... Args>
size_t meshopt_simplify_func(bool sloppy, Args... args)
{
    if (sloppy)
        return meshopt_simplifySloppy(args...);
    else
        return meshopt_simplify(args...);
}

void LoopSubface::MeshoptDecimate(int level, bool sloppy)
{
    std::string func_name = fmt::format("LoopSubface::MeshoptDecimate(level={}, sloppy={})", level, sloppy);
    Timer timer(func_name);

    if (level >= 0)
        level_ = level;

    size_t index_count = origin_indexes_.size();
    size_t position_count = origin_positions_.size();
    float threshold = (1 <= level_ && level_ <= 9) ? (1.f - level_ * 0.1f) : 1.f;
    /* Hardcoded threshold table. */
    // float threshold_table[] { 1.f, 0.5f, 0.25f, 0.125f, 0.0625f, 0.03125f };
    // threshold = threshold_table[level % (sizeof(threshold_table) / sizeof(float))];
    size_t target_index_count = static_cast<size_t>(index_count * threshold);
    if (level == -1)
        target_index_count = std::min((result_face_count_ + 1) * 3, index_count);
    else if (level == -2)
        target_index_count = std::max(size_t(1), result_face_count_) * 3 - 3;
    float target_error = 1.f;

    std::vector<uint32_t> result_indexes(index_count);
    size_t result_index_count = 0;
    // Use meshopt_simplify_func() as a proxy to prevent duplicated code (writing those many parameters for both functions).
    if (level == -1) {
        size_t target_index_count_temp = target_index_count;
        // Meshopt simplifies index count to a round-down number. To ensure we can increase the face count successfully,
        // we progressively increase `target_index_count_temp` to get `result_index_count` larger than `target_index_count`.
        while (true) {
            result_index_count = meshopt_simplify_func(sloppy, &result_indexes[0], &origin_indexes_[0], index_count,
                &origin_positions_[0].x, position_count, sizeof(glm::vec3),
                target_index_count_temp, target_error);
            if (result_index_count < target_index_count)
                target_index_count_temp += 3;
            else
                break;
        }
    } else {
        result_index_count = meshopt_simplify_func(sloppy, &result_indexes[0], &origin_indexes_[0], index_count,
            &origin_positions_[0].x, position_count, sizeof(glm::vec3),
            target_index_count, target_error);
    }
    result_face_count_ = result_index_count / 3;
    result_indexes.resize(result_index_count);

    std::vector<glm::vec3> result_positions(position_count);
    size_t result_position_count = 0;
    // `result_index_count` may be 0 meaning all the triangles are decimated.
    if (result_index_count)
        result_position_count = meshopt_optimizeVertexFetch(&result_positions[0].x, &result_indexes[0], result_index_count,
            &origin_positions_[0].x, position_count, sizeof(glm::vec3));
    result_positions.resize(result_position_count);

    std::vector<Vertex> vertexes;
    std::vector<Face> faces;
    BuildTopology(result_positions, result_indexes, vertexes, faces);

    std::vector<Vertex*> vertexes_base(vertexes.size());
    std::vector<Face*> faces_base(faces.size());
    for (size_t i = 0; i < vertexes.size(); i++)
        vertexes_base[i] = &vertexes[i];
    for (size_t i = 0; i < faces.size(); i++)
        faces_base[i] = &faces[i];
    ComputeNormalsAndPositions(vertexes_base, faces_base);

    spdlog::info("{}: {} triangles, {} vertexes", func_name, faces_base.size(), vertexes_base.size());
}

struct QueueEdge {
    const Vertex* const v[2];
    const float l;

    QueueEdge(const Vertex* v0, const Vertex* v1)
        : v { std::min(v0, v1), std::max(v0, v1) }
        , l(glm::distance(v[0]->p, v[1]->p))
    {
    }

    bool operator<(const QueueEdge& qe) const
    {
        const std::array<void*, 2>& a = *reinterpret_cast<const std::array<void*, 2>*>(v);
        const std::array<void*, 2>& b = *reinterpret_cast<const std::array<void*, 2>*>(qe.v);
        return l < qe.l || l == qe.l && a < b;
    }
};

bool CollapseEdge(std::vector<Vertex>& vertexes, std::vector<Face>& faces, std::set<QueueEdge>& queue, size_t& decimate_face_count,
    size_t target_face_count, bool round_down, bool midpoint)
{
    const QueueEdge collapse_e = *queue.begin();
    queue.erase(queue.begin());

    Vertex* v0 = const_cast<Vertex*>(collapse_e.v[0]);
    Vertex* v1 = const_cast<Vertex*>(collapse_e.v[1]);

    std::vector<const Face*> sweep = v1->OneSweep();
    int face_count_to_decimate_for_this_collapse = 0;
    if (!round_down) {
        for (const Face* f : sweep)
            if (f->VertexId(v0) != -1)
                ++face_count_to_decimate_for_this_collapse;
        if (decimate_face_count - face_count_to_decimate_for_this_collapse < target_face_count)
            return false;
    }

    std::vector<const Vertex*> ring[2] { v0->OneRing(), v1->OneRing() };

    std::vector<Face*> collapse_f;
    std::vector<Vertex*> collapse_f_v;

    for (const Face* f_const : sweep) {
        Face* f = const_cast<Face*>(f_const);
        int v1_id = f->VertexId(v1);
        int v0_id = f->VertexId(v0);

        if (v0_id != -1) {
            collapse_f.push_back(f);
            int v2_id = 3 - v0_id - v1_id;
            Vertex* v2 = const_cast<Vertex*>(f->v[v2_id]);
            collapse_f_v.push_back(v2);
            Face* fn[2] { const_cast<Face*>(f->neighbors[v2_id]), const_cast<Face*>(f->neighbors[PREV(v2_id)]) };

            for (int i = 0; i < 2; ++i)
                if (fn[i] && fn[i] != fn[1 - i] && fn[i]->VertexId(v0) != -1 && fn[i]->VertexId(v1) != -1) {
                    int k = NEXT(fn[i]->VertexId(v1));
                    if (fn[i]->neighbors[k] == f)
                        k = NEXT(fn[i]->VertexId(v0));
                    assert(fn[i]->neighbors[k] != f);
                    for (int l = 0; fn[i]->neighbors[k] && l < 3; ++l)
                        if (fn[i]->neighbors[k]->neighbors[l] == fn[i]->neighbors[k])
                            const_cast<Face*>(fn[i]->neighbors[k])->neighbors[l] = fn[1 - i];
                    for (int l = 0; fn[1 - i] && l < 3; ++l)
                        if (fn[1 - i]->neighbors[l] == f)
                            fn[1 - i]->neighbors[l] = fn[i]->neighbors[k];
                    if (v2->start_face == f || v2->start_face == fn[i])
                        v2->start_face = fn[1 - i] ? fn[1 - i] : fn[i]->neighbors[k];
                }

            for (int i = 0; i < 2; ++i)
                if (fn[i] && (fn[i]->VertexId(v0) == -1 || fn[i]->VertexId(v1) == -1))
                    for (int j = 0; j < 3; ++j)
                        if (fn[i]->neighbors[j] == f) {
                            fn[i]->neighbors[j] = fn[1 - i];
                        }
            if (v2->start_face == f)
                v2->start_face = fn[0] ? fn[0] : fn[1];

            QueueEdge e(v1, v2);
            auto e_it = queue.find(e);
            if (e_it != queue.end())
                queue.erase(e_it);

            // `Face::children[k]` are initialized as `nullptr`. Use the first child to flag deletion.
            f->children[0] = reinterpret_cast<Face*>(1);
            decimate_face_count--;
        }
    }

    // Should always seperate updates of faces' neighbors and vertexes to reduce the possibility of bugs.
    // Update faces' neighbors first, then vertexes.
    for (const Face* f_const : sweep) {
        Face* f = const_cast<Face*>(f_const);
        int v1_id = f->VertexId(v1);
        f->v[v1_id] = v0;
    }

    if (midpoint) {
        std::vector<const Vertex*> ring_remain;
        Vertex* v01[2] { v0, v1 };
        for (int i = 0; i < 2; ++i)
            for (auto v : ring[i]) {
                auto e_it = queue.find({ v, v01[i] });
                if (e_it != queue.end()) {
                    queue.erase(e_it);
                    ring_remain.push_back(v);
                }
            }
        if (midpoint)
            v0->p = (v0->p + v1->p) * 0.5f;
        for (auto v : ring_remain)
            queue.insert({ v, v0 });
    } else {
        for (auto v : ring[1]) {
            auto e_it = queue.find({ v, v1 });
            if (e_it != queue.end()) {
                queue.erase(e_it);
                queue.insert({ v, v0 });
            }
        }
    }

    auto f_it = std::find(collapse_f.begin(), collapse_f.end(), v0->start_face);
    if (f_it != collapse_f.end()) {
        v0->start_face = nullptr;
        for (int i = 0; i < collapse_f.size(); ++i)
            if (collapse_f[i] && (v0->start_face == nullptr || v0->start_face->children[0])) {
                v0->start_face = collapse_f[i]->PrevNeighbor(collapse_f_v[i]);
                if (v0->start_face == nullptr || v0->start_face->children[0])
                    v0->start_face = collapse_f[i]->NextNeighbor(collapse_f_v[i]);
            }
    }
    if (v0->start_face && v0->start_face->children[0] == nullptr) {
        v0->ComputeStartFaceAndBoundary();
        v0->ComputeValence();
    } else {
        for (auto v : ring[0]) {
            auto e_it = queue.find({ v, v0 });
            if (e_it != queue.end())
                queue.erase(e_it);
        }
        v0->child = reinterpret_cast<Vertex*>(1);
    }

    std::sort(collapse_f_v.begin(), collapse_f_v.end());
    auto v_it_end = std::unique(collapse_f_v.begin(), collapse_f_v.end());
    for (auto v_it = collapse_f_v.begin(); v_it != v_it_end; ++v_it)
        if ((*v_it)->start_face && (*v_it)->start_face->children[0] == nullptr) {
            (*v_it)->ComputeValence();
        } else {
            auto e_it = queue.find({ *v_it, v0 });
            if (e_it != queue.end())
                queue.erase(e_it);
            (*v_it)->child = reinterpret_cast<Vertex*>(1);

            /* Delete all the edges with the degenerated v2. */
            // std::vector<QueueEdge> collapse_f_v_e;
            // for (const QueueEdge& e : queue) {
            //     // assert(e.v[0] != *v_it && e.v[1] != *v_it);
            //     for (int i = 0; i < 2; ++i)
            //         if (e.v[i] == *v_it) {
            //             collapse_f_v_e.push_back(e);
            //             break;
            //         }
            // }
            // for (const QueueEdge& e : collapse_f_v_e)
            //     queue.erase(e);
        }

    // `Vertex::child` is initialized as `nullptr`. Use it to flag deletion.
    v1->child = reinterpret_cast<Vertex*>(1);

    return face_count_to_decimate_for_this_collapse;
}

void LoopSubface::Decimate(int level, bool midpoint)
{
    std::string func_name = fmt::format("LoopSubface::Decimate(level={})", level);
    Timer timer(func_name);

    if (level >= 0)
        level_ = level;

    std::vector<Vertex> vertexes;
    std::vector<Face> faces;
    BuildTopology(origin_positions_, origin_indexes_, vertexes, faces);
    size_t vertex_count = vertexes_.size();
    size_t face_count = faces_.size();

    float threshold = (1 <= level_ && level_ <= 9) ? (1.f - level_ * 0.1f) : 1.f;
    size_t target_face_count = static_cast<size_t>(faces_.size() * threshold);
    if (level == -1)
        target_face_count = result_face_count_ + 1;
    else if (level == -2)
        target_face_count = std::max(size_t(1), result_face_count_) - 1;

    std::set<QueueEdge> queue;
    for (auto& f : faces)
        for (int vi = 0; vi < 3; ++vi)
            queue.insert({ f.v[vi], f.v[NEXT(vi)] });

    size_t decimate_face_count = face_count;
    if (level == -1) {
        // An edge collapse may decimate more than 1 face (2 usually, 1 for border edges, >2 for corner cases).
        // Use round up mode (with the last parameter `round_down==false`) here to ensure we can increase the face count successfully.
        while (decimate_face_count > target_face_count && CollapseEdge(vertexes, faces, queue, decimate_face_count, target_face_count, false, midpoint))
            ;
    } else {
        while (decimate_face_count > target_face_count)
            CollapseEdge(vertexes, faces, queue, decimate_face_count, target_face_count, true, midpoint);
    }
    result_face_count_ = decimate_face_count;

    std::vector<Vertex*> vertexes_base;
    std::vector<Face*> faces_base;
    for (size_t i = 0; i < faces.size(); i++)
        if (faces[i].children[0] == nullptr)
            faces_base.push_back(&faces[i]);
    if (!faces_base.empty())
        for (size_t i = 0; i < vertexes.size(); i++)
            if (vertexes[i].child == nullptr)
                vertexes_base.push_back(&vertexes[i]);
    ComputeNormalsAndPositions(vertexes_base, faces_base);

    spdlog::info("{}: {} triangles, {} vertexes", func_name, faces_base.size(), vertexes_base.size());
}

void LoopSubface::ExportObj(const std::string& file_name, bool smooth) const
{
    std::string func_name = fmt::format("LoopSubface::ExportObj(file_name={}, smooth={})", file_name, smooth);
    Timer timer(func_name);

    std::ofstream ofs(file_name);

    for (auto& v : indexed_positions_)
        ofs << "v " << v.x << " " << v.y << " " << v.z << std::endl;
    if (smooth) {
        for (auto& n : indexed_smooth_normals_)
            ofs << "vn " << n.x << " " << n.y << " " << n.z << std::endl;
        for (size_t i = 0; i < vertex_indexes_.size(); i += 3)
            ofs << "f "
                << vertex_indexes_[i + 0] + 1 << "//" << smooth_normal_indexes_[i + 0] + 1 << " "
                << vertex_indexes_[i + 1] + 1 << "//" << smooth_normal_indexes_[i + 1] + 1 << " "
                << vertex_indexes_[i + 2] + 1 << "//" << smooth_normal_indexes_[i + 2] + 1 << std::endl;
    } else {
        for (auto& n : indexed_flat_normals_)
            ofs << "vn " << n.x << " " << n.y << " " << n.z << std::endl;
        for (size_t i = 0; i < vertex_indexes_.size(); i += 3)
            ofs << "f "
                << vertex_indexes_[i + 0] + 1 << "//" << flat_normal_indexes_[i + 0] + 1 << " "
                << vertex_indexes_[i + 1] + 1 << "//" << flat_normal_indexes_[i + 1] + 1 << " "
                << vertex_indexes_[i + 2] + 1 << "//" << flat_normal_indexes_[i + 2] + 1 << std::endl;
    }

    spdlog::info("{}: Mesh exported: {}", func_name, file_name);
}

}
