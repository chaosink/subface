#include "LoopSubface.hpp"

#include <fstream>
#include <iostream>
#include <map>
#include <set>

#include <spdlog/spdlog.h>

#include "Timer.hpp"

namespace subface {

void LoopSubface::BuildTopology(const std::vector<glm::vec3>& positions, const std::vector<uint32_t>& indexes,
    std::vector<Vertex>& vertexes, std::vector<Face>& faces)
{
    size_t n_vertexes = positions.size();
    size_t n_faces = indexes.size() / 3;

    vertexes.resize(n_vertexes);
    for (int i = 0; i < n_vertexes; ++i)
        vertexes[i].p = positions[i];

    faces.resize(n_faces);
    for (int i = 0; i < n_faces; ++i)
        for (int j = 0; j < 3; j++) {
            faces[i].v[j] = &vertexes[indexes[i * 3 + j]];
            // `start_face` of the same vertex may be updated multiple times.
            vertexes[indexes[i * 3 + j]].start_face = &faces[i];
        }

    // A local variable for temp usage.
    std::set<Edge> edges;
    for (int i = 0; i < n_faces; ++i) {
        Face* f = &faces[i];
        for (int vi = 0; vi < 3; ++vi) {
            int v0 = vi, v1 = NEXT(vi);
            Edge e(f->v[v0], f->v[v1]);
            auto e_it = edges.find(e);
            if (e_it == edges.end()) {
                e.f = f;
                e.id = vi;
                edges.insert(e);
            } else {
                e_it->f->neighbors[e_it->id] = f;
                f->neighbors[vi] = e_it->f;
                edges.erase(e_it);
            }
        }
    }

    for (int i = 0; i < n_vertexes; ++i) {
        Vertex* v = &vertexes[i];
        const Face *f = v->start_face, *start_face_old = v->start_face;
        do {
            // Update `start_face`, especially for boundary vertexes.
            v->start_face = f;
            f = f->PrevNeighbor(v);
        } while (f && f != start_face_old);
        v->boundary = (f == nullptr);

        v->ComputeValence();
        //   \ /   //
        // -- . -- //
        //   / \   //
        if (!v->boundary && v->valence == 6)
            v->regular = true;
        //   \ /   //
        // -- . -- //
        else if (v->boundary && v->valence == 4)
            v->regular = true;
        else
            v->regular = false;
    }
}

void LoopSubface::BuildTopology(const std::vector<glm::vec3>& positions, const std::vector<uint32_t>& indexes)
{
    origin_positions_ = positions;
    origin_indexes_ = indexes;

    BuildTopology(origin_positions_, origin_indexes_, vertexes_, faces_);
}

// Only for non-boundary vertexes.
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
    std::vector<glm::vec3> ring = v->OneRing();
    glm::vec3 p = (1 - valence * beta) * v->p;
    for (int i = 0; i < valence; ++i)
        p += beta * ring[i];
    return p;
}

glm::vec3 LoopSubface::WeightBoundary(Vertex* v, float beta)
{
    std::vector<glm::vec3> boundary_neighbors = v->BoundaryNeighbors();
    glm::vec3 p = (1 - beta * 2.f) * v->p + (boundary_neighbors[0] + boundary_neighbors[1]) * beta;
    return p;
}

void LoopSubface::ComputeNormalsAndPositions(const std::vector<Vertex*>& vertexes_base, const std::vector<Face*>& faces_base)
{
    // Compute vertexes' smooth normals.
    std::vector<glm::vec3> smooth_normals;
    smooth_normals.reserve(vertexes_base.size());
    std::vector<glm::vec3> ring;
    for (const Vertex* v : vertexes_base) {
        glm::vec3 S(0, 0, 0), T(0, 0, 0);
        int valence = v->valence;
        ring = v->OneRing();
        if (!v->boundary) {
            for (int i = 0; i < valence; ++i) {
                T += std::cos(2.f * PI * i / valence) * ring[i];
                S += std::sin(2.f * PI * i / valence) * ring[i];
            }
        } else {
            S = ring[valence - 1] - ring[0];
            if (valence == 2)
                T = -v->p * 2.f + ring[0] + ring[1];
            else if (valence == 3)
                T = -v->p + ring[1];
            else if (valence == 4)
                T = -v->p * 2.f - ring[0] + ring[1] * 2.f + ring[2] * 2.f - ring[3];
            else {
                float theta = PI / float(valence - 1);
                T = std::sin(theta) * (ring[0] + ring[valence - 1]);
                for (int i = 1; i < valence - 1; ++i) {
                    float weight = (std::cos(theta) * 2.f - 2.f) * std::sin(theta * i);
                    T += ring[i] * weight;
                }
                T = -T;
            }
        }
        smooth_normals.push_back(glm::normalize(glm::cross(S, T)));
    }

    // Vertex indexes.
    std::map<const Vertex*, int> vertex2index;
    for (size_t i = 0; i < vertexes_base.size(); ++i)
        vertex2index[vertexes_base[i]] = static_cast<int>(i);

    indexed_positions_.resize(vertexes_base.size());
    for (size_t i = 0; i < vertexes_base.size(); ++i)
        indexed_positions_[i] = vertexes_base[i]->p;

    vertex_indexes_.resize(faces_base.size() * 3);
    smooth_normal_indexes_.resize(faces_base.size() * 3);
    flat_normal_indexes_.resize(faces_base.size() * 3);
    for (size_t i = 0; i < faces_base.size(); ++i)
        for (int j = 0; j < 3; ++j) {
            vertex_indexes_[i * 3 + j] = vertex2index[faces_base[i]->v[j]];
            smooth_normal_indexes_[i * 3 + j] = vertex2index[faces_base[i]->v[j]];
        }
    indexed_flat_normals_.resize(faces_base.size());

    // Unindexd vertexes and normals.
    unindexed_positions_.resize(faces_base.size() * 3);
    unindexed_smooth_normals_.resize(faces_base.size() * 3);
    unindexed_flat_normals_.resize(faces_base.size() * 3);
    for (size_t i = 0; i < faces_base.size(); i++) {
        glm::vec3 normal_flat = glm::normalize(glm::cross(
            faces_base[i]->v[1]->p - faces_base[i]->v[0]->p,
            faces_base[i]->v[2]->p - faces_base[i]->v[1]->p));
        for (int j = 0; j < 3; j++) {
            unindexed_positions_[i * 3 + j] = faces_base[i]->v[j]->p;
            unindexed_smooth_normals_[i * 3 + j] = smooth_normals[vertex2index[faces_base[i]->v[j]]];
            unindexed_flat_normals_[i * 3 + j] = normal_flat;
            flat_normal_indexes_[i * 3 + j] = static_cast<int>(i);
        }
        indexed_flat_normals_[i] = normal_flat;
    }

    indexed_smooth_normals_ = std::move(smooth_normals);
}

// Same as Tesselate4(int level).
void LoopSubface::Subdivide(int level, bool flat)
{
    Timer timer("LoopSubface::Subdivide()");

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
                    // -- . -- //
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
                        //     .   //
                        //    / \  //
                        //   .-o-. //
                        //    \ /  //
                        //     .   //
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

        // Update new sub-faces' neighbors.
        for (auto& f : faces_base) {
            for (int ci = 0; ci < 3; ++ci) {
                //     1
                //    /1\
                //   0 - 1
                //  /0\3/2\
                // 0 - 2 - 2
                // `neighbors[i]` is the face sharing the edge of v[i] and v[NEXT(i)].
                f->children[3]->neighbors[ci] = f->children[NEXT(ci)];
                f->children[ci]->neighbors[NEXT(ci)] = f->children[3];

                const Face* fn = f->neighbors[ci];
                f->children[ci]->neighbors[ci] = fn ? fn->children[fn->VertexId(f->v[ci])] : nullptr;
                fn = f->neighbors[PREV(ci)];
                f->children[ci]->neighbors[PREV(ci)] = fn ? fn->children[fn->VertexId(f->v[ci])] : nullptr;
            }
        }

        // Update new sub-faces' vertexes.
        for (auto& f : faces_base) {
            for (int ci = 0; ci < 3; ++ci) {
                f->children[ci]->v[ci] = f->v[ci]->child;

                // 3 new sub-faces share the the same new sub-vertex.
                Vertex* vertex = edge2vertex[Edge(f->v[ci], f->v[NEXT(ci)])];
                f->children[ci]->v[NEXT(ci)] = vertex;
                f->children[NEXT(ci)]->v[ci] = vertex;
                f->children[3]->v[ci] = vertex;
            }
        }

        // All updates done. Replace the base with the new for further subdivisions.
        vertexes_base = std::move(vertexes_new);
        faces_base = std::move(faces_new);
    }

    ComputeNormalsAndPositions(vertexes_base, faces_base);

    spdlog::info("Subdivision level {}: {} triangles, {} vertices", level_, unindexed_positions_.size() / 3, unindexed_positions_.size());
}

void LoopSubface::Tesselate3(int level)
{
    Timer timer("LoopSubface::Tesselate3()");

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
            // `regular` is useless for tesselation.
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
            v->child->start_face = v->start_face->children[PREV(vi)];
        }

        // Update new sub-faces' neighbors.
        for (auto& f : faces_base) {
            for (int ci = 0; ci < 3; ++ci) {
                // `neighbors[i]` is the face sharing the edge of v[i] and v[NEXT(i)].
                f->children[ci]->neighbors[PREV(ci)] = f->children[PREV(ci)];
                f->children[ci]->neighbors[NEXT(ci)] = f->children[NEXT(ci)];

                const Face* fn = f->neighbors[ci];
                f->children[ci]->neighbors[ci] = fn ? fn->children[fn->VertexId(f->v[NEXT(ci)])] : nullptr;
            }
        }

        // Update new sub-faces' vertexes.
        for (size_t fi = 0; fi < faces_base.size(); ++fi) {
            Face* f = faces_base[fi];
            for (int ci = 0; ci < 3; ++ci) {
                f->children[ci]->v[ci] = f->v[ci]->child;
                f->children[ci]->v[NEXT(ci)] = f->v[NEXT(ci)]->child;
                f->children[ci]->v[PREV(ci)] = vertexes_new[vertexes_base.size() + fi];
            }
        }

        // All updates done. Replace the base with the new for further subdivisions.
        vertexes_base = std::move(vertexes_new);
        faces_base = std::move(faces_new);
    }

    ComputeNormalsAndPositions(vertexes_base, faces_base);

    spdlog::info("Tesselate3 level {}: {} triangles, {} vertices", level_, unindexed_positions_.size() / 3, unindexed_positions_.size());
}

// Same as Subdivide(int level, bool flat = true).
void LoopSubface::Tesselate4(int level)
{
    Timer timer("LoopSubface::Tesselate4()");

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

        // Update new sub-faces' neighbors.
        for (auto& f : faces_base) {
            for (int ci = 0; ci < 3; ++ci) {
                //     1
                //    /1\
                //   0 - 1
                //  /0\3/2\
                // 0 - 2 - 2
                // `neighbors[i]` is the face sharing the edge of v[i] and v[NEXT(i)].
                f->children[3]->neighbors[ci] = f->children[NEXT(ci)];
                f->children[ci]->neighbors[NEXT(ci)] = f->children[3];

                const Face* fn = f->neighbors[ci];
                f->children[ci]->neighbors[ci] = fn ? fn->children[fn->VertexId(f->v[ci])] : nullptr;
                fn = f->neighbors[PREV(ci)];
                f->children[ci]->neighbors[PREV(ci)] = fn ? fn->children[fn->VertexId(f->v[ci])] : nullptr;
            }
        }

        // Update new sub-faces' vertexes.
        for (auto& f : faces_base) {
            for (int ci = 0; ci < 3; ++ci) {
                f->children[ci]->v[ci] = f->v[ci]->child;

                // 3 new sub-faces share the the same new sub-vertex.
                Vertex* vertex = edge2vertex[Edge(f->v[ci], f->v[NEXT(ci)])];
                f->children[ci]->v[NEXT(ci)] = vertex;
                f->children[NEXT(ci)]->v[ci] = vertex;
                f->children[3]->v[ci] = vertex;
            }
        }

        // All updates done. Replace the base with the new for further subdivisions.
        vertexes_base = std::move(vertexes_new);
        faces_base = std::move(faces_new);
    }

    ComputeNormalsAndPositions(vertexes_base, faces_base);

    spdlog::info("Tesselate4 level {}: {} triangles, {} vertices", level_, unindexed_positions_.size() / 3, unindexed_positions_.size());
}

void LoopSubface::Tesselate4_1(int level)
{
    Timer timer("LoopSubface::Tesselate4_1()");

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
            // `regular` is useless for tesselation.
            v->child->p = v->p;
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

        // Update new sub-faces' neighbors.
        for (auto& f : faces_base) {
            //     1
            //    /|\
            //   01|21
            //  /0\|/3\
            // 0 - 2 - 2
            // `neighbors[i]` is the face sharing the edge of v[i] and v[NEXT(i)].

            const Face* f0 = f->neighbors[0];
            const Face* f1 = f->neighbors[1];
            const Face* f2 = f->neighbors[2];

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

            f->children[0]->neighbors[0] = f0 ? f0->children[vertex_id_to_child_id_pre(f0->VertexId(f->v[0]))] : nullptr;
            f->children[0]->neighbors[1] = f->children[1];
            f->children[0]->neighbors[2] = f2 ? f2->children[vertex_id_to_child_id_nxt(f2->VertexId(f->v[0]))] : nullptr;

            f->children[1]->neighbors[0] = f0 ? f0->children[vertex_id_to_child_id_nxt(f0->VertexId(f->v[1]))] : nullptr;
            f->children[1]->neighbors[1] = f->children[2];
            f->children[1]->neighbors[2] = f->children[0];

            f->children[2]->neighbors[0] = f->children[1];
            f->children[2]->neighbors[1] = f1 ? f1->children[vertex_id_to_child_id_pre(f1->VertexId(f->v[1]))] : nullptr;
            f->children[2]->neighbors[2] = f->children[3];

            f->children[3]->neighbors[0] = f->children[2];
            f->children[3]->neighbors[1] = f1 ? f1->children[vertex_id_to_child_id_nxt(f1->VertexId(f->v[2]))] : nullptr;
            f->children[3]->neighbors[2] = f2 ? f2->children[vertex_id_to_child_id_pre(f2->VertexId(f->v[2]))] : nullptr;
        }

        // Update new sub-faces' vertexes.
        for (auto& f : faces_base) {
            f->v[1]->child->valence += 1;

            Edge e0 { f->v[0], f->v[1] };
            Edge e1 { f->v[1], f->v[2] };
            Edge e2 { f->v[2], f->v[0] };

            f->children[0]->v[0] = f->v[0]->child;
            f->children[0]->v[1] = edge2vertex[e0];
            f->children[0]->v[2] = edge2vertex[e2];

            f->children[1]->v[0] = edge2vertex[e0];
            f->children[1]->v[1] = f->v[1]->child;
            f->children[1]->v[2] = edge2vertex[e2];

            f->children[2]->v[0] = edge2vertex[e2];
            f->children[2]->v[1] = f->v[1]->child;
            f->children[2]->v[2] = edge2vertex[e1];

            f->children[3]->v[0] = edge2vertex[e2];
            f->children[3]->v[1] = edge2vertex[e1];
            f->children[3]->v[2] = f->v[2]->child;
        }

        // All updates done. Replace the base with the new for further subdivisions.
        vertexes_base = std::move(vertexes_new);
        faces_base = std::move(faces_new);
    }

    ComputeNormalsAndPositions(vertexes_base, faces_base);

    spdlog::info("Tesselate4_1 level {}: {} triangles, {} vertices", level_, unindexed_positions_.size() / 3, unindexed_positions_.size());
}

void LoopSubface::ExportObj(std::string file_name, bool smooth) const
{
    file_name = file_name.substr(0, file_name.size() - 4) + "_loop-" + char('0' + level_);
    if (smooth)
        file_name += "_smooth.obj";
    else
        file_name += "_flat.obj";
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

    spdlog::info("Mesh exported: {}", file_name);
}

}
