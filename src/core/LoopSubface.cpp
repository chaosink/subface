#include "LoopSubface.hpp"

#include <fstream>
#include <iostream>
#include <map>
#include <set>

#include <spdlog/spdlog.h>

#include "Timer.hpp"

namespace subface {

void LoopSubface::BuildTopology(const std::vector<glm::vec3>& vertexes, const std::vector<int>& indexes)
{
    size_t n_vertexes = vertexes.size();
    size_t n_faces = indexes.size() / 3;

    vertexes_.resize(n_vertexes);
    for (int i = 0; i < n_vertexes; ++i)
        vertexes_[i].p = vertexes[i];

    faces_.resize(n_faces);
    for (int i = 0; i < n_faces; ++i)
        for (int j = 0; j < 3; j++) {
            faces_[i].v[j] = &vertexes_[indexes[i * 3 + j]];
            vertexes_[indexes[i * 3 + j]].start_face = &faces_[i];
        }

    std::set<Edge> edges;
    for (int i = 0; i < n_faces; ++i) {
        Face* f = &faces_[i];
        for (int edge_num = 0; edge_num < 3; ++edge_num) {
            int v0 = edge_num, v1 = NEXT(edge_num);
            Edge e(f->v[v0], f->v[v1]);
            if (edges.find(e) == edges.end()) {
                e.f = f;
                e.edge_num = edge_num;
                edges.insert(e);
            } else {
                e = *edges.find(e);
                e.f->neighbors[e.edge_num] = f;
                f->neighbors[edge_num] = e.f;
                edges.erase(e);
            }
        }
    }

    for (int i = 0; i < n_vertexes; ++i) {
        Vertex* v = &vertexes_[i];
        const Face* f = v->start_face;
        do {
            f = f->NextNeighbor(v);
        } while (f && f != v->start_face);
        v->boundary = (f == nullptr);

        int valence = v->Valence();
        if (!v->boundary && valence == 6)
            v->regular = true;
        else if (v->boundary && valence == 4)
            v->regular = true;
        else
            v->regular = false;
    }
}

float LoopSubface::Beta(int valence)
{
    if (valence == 3)
        return 3.f / 16.f;
    else
        return 3.f / (8.f * valence);
}

float LoopSubface::LoopGamma(int valence)
{
    return 1.f / (valence + 3.f / (8.f * Beta(valence)));
}

glm::vec3 LoopSubface::WeightOneRing(Vertex* v, float beta)
{
    int valence = v->Valence();
    std::vector<glm::vec3> ring(valence);
    v->OneRing(ring);
    glm::vec3 p = (1 - valence * beta) * v->p;
    for (int i = 0; i < valence; ++i)
        p += beta * ring[i];
    return p;
}

glm::vec3 LoopSubface::WeightBoundary(Vertex* v, float beta)
{
    int valence = v->Valence();
    std::vector<glm::vec3> ring(valence);
    v->OneRing(ring);
    glm::vec3 p = (1 - 2 * beta) * v->p;
    p += beta * ring[0];
    p += beta * ring[valence - 1];
    return p;
}

void LoopSubface::Subdivide(int level)
{
    Timer timer("LoopSubface::Subdivide()");

    level_ = level;

    std::vector<Vertex*> v(vertexes_.size());
    std::vector<Face*> f(faces_.size());
    for (size_t i = 0; i < vertexes_.size(); i++)
        v[i] = &vertexes_[i];
    for (size_t i = 0; i < faces_.size(); i++)
        f[i] = &faces_[i];

    MemoryPool mp;
    for (int i = 0; i < level; ++i) {
        std::vector<Vertex*> new_vertexes;
        std::vector<Face*> new_faces(f.size() * 4);

        for (auto& vertex : v) {
            new_vertexes.emplace_back(mp.New<Vertex>());
            vertex->child = new_vertexes.back();
            vertex->child->regular = vertex->regular;
            vertex->child->boundary = vertex->boundary;
        }
        for (size_t j = 0; j < f.size(); ++j)
            for (int k = 0; k < 4; ++k)
                f[j]->children[k] = new_faces[j * 4 + k] = mp.New<Face>();

        for (auto& vertex : v)
            if (!vertex->boundary) {
                if (vertex->regular)
                    vertex->child->p = WeightOneRing(vertex, 1.f / 16.f);
                else
                    vertex->child->p = WeightOneRing(vertex, Beta(vertex->Valence()));
            } else {
                vertex->child->p = WeightBoundary(vertex, 1.f / 8.f);
            }

        std::map<Edge, Vertex*> edge_vertex;
        for (auto& face : f) {
            for (int k = 0; k < 3; ++k) {
                Edge edge(face->v[k], face->v[NEXT(k)]);
                Vertex* vertex = edge_vertex[edge];
                if (!vertex) {
                    new_vertexes.emplace_back(mp.New<Vertex>());
                    vertex = new_vertexes.back();
                    vertex->regular = true;
                    vertex->boundary = (face->neighbors[k] == nullptr);
                    vertex->start_face = face->children[3];

                    if (vertex->boundary) {
                        vertex->p = 0.5f * edge.v[0]->p;
                        vertex->p += 0.5f * edge.v[1]->p;
                    } else {
                        vertex->p = 3.f / 8.f * edge.v[0]->p;
                        vertex->p += 3.f / 8.f * edge.v[1]->p;
                        vertex->p += 1.f / 8.f
                            * face->OtherVertex(edge.v[0], edge.v[1])->p;
                        vertex->p += 1.f / 8.f
                            * face->neighbors[k]->OtherVertex(edge.v[0], edge.v[1])->p;
                    }
                    edge_vertex[edge] = vertex;
                }
            }
        }

        for (auto& vertex : v) {
            int v_num = vertex->start_face->VNum(vertex);
            vertex->child->start_face = vertex->start_face->children[v_num];
        }

        for (auto& face : f) {
            for (int j = 0; j < 3; ++j) {
                face->children[3]->neighbors[j] = face->children[NEXT(j)];
                face->children[j]->neighbors[NEXT(j)] = face->children[3];

                const Face* f2 = face->neighbors[j];
                face->children[j]->neighbors[j] = f2 ? f2->children[f2->VNum(face->v[j])] : nullptr;
                f2 = face->neighbors[PREV(j)];
                face->children[j]->neighbors[PREV(j)] = f2 ? f2->children[f2->VNum(face->v[j])] : nullptr;
            }
        }

        for (auto& face : f) {
            for (int j = 0; j < 3; ++j) {
                face->children[j]->v[j] = face->v[j]->child;

                Vertex* vertex = edge_vertex[Edge(face->v[j], face->v[NEXT(j)])];
                face->children[j]->v[NEXT(j)] = vertex;
                face->children[NEXT(j)]->v[j] = vertex;
                face->children[3]->v[j] = vertex;
            }
        }

        v = std::move(new_vertexes);
        f = std::move(new_faces);
    }

    if (level) {
        std::vector<glm::vec3> limit(v.size());
        for (size_t i = 0; i < v.size(); ++i) {
            if (v[i]->boundary)
                limit[i] = WeightBoundary(v[i], 1.f / 5.f);
            else
                limit[i] = WeightOneRing(v[i], LoopGamma(v[i]->Valence()));
        }
        for (size_t i = 0; i < v.size(); ++i)
            v[i]->p = limit[i];
    }

    std::vector<glm::vec3> normals;
    normals.reserve(v.size());
    std::vector<glm::vec3> ring;
    for (Vertex* vertex : v) {
        glm::vec3 S(0, 0, 0), T(0, 0, 0);
        int valence = vertex->Valence();
        ring.resize(valence);
        vertex->OneRing(ring);
        if (!vertex->boundary) {
            for (int j = 0; j < valence; ++j) {
                T += std::cos(2 * PI * j / valence) * ring[j];
                S += std::sin(2 * PI * j / valence) * ring[j];
            }
        } else {
            S = ring[valence - 1] - ring[0];
            if (valence == 2)
                T = ring[0] + ring[1] - vertex->p * 2.f;
            else if (valence == 3)
                T = ring[1] - vertex->p;
            else if (valence == 4)
                T = -1.f * ring[0] + 2.f * ring[1] + 2.f * ring[2] + -1.f * ring[3] - 2.f * vertex->p;
            else {
                float theta = PI / float(valence - 1);
                T = std::sin(theta) * (ring[0] + ring[valence - 1]);
                for (int k = 1; k < valence - 1; ++k) {
                    float wt = (2 * std::cos(theta) - 2) * std::sin((k)*theta);
                    T += wt * ring[k];
                }
                T = -T;
            }
        }
        normals.push_back(glm::normalize(glm::cross(S, T)));
    }

    std::map<const Vertex*, int> vertex_index;
    for (size_t i = 0; i < v.size(); ++i)
        vertex_index[v[i]] = static_cast<int>(i);

    indexed_vertex_.resize(v.size());
    for (size_t i = 0; i < v.size(); ++i)
        indexed_vertex_[i] = v[i]->p;

    index_vertex_.resize(f.size() * 3);
    index_normal_smooth_.resize(f.size() * 3);
    index_normal_flat_.resize(f.size() * 3);
    for (size_t i = 0; i < f.size(); ++i)
        for (int j = 0; j < 3; j++) {
            index_vertex_[i * 3 + j] = vertex_index[f[i]->v[j]];
            index_normal_smooth_[i * 3 + j] = vertex_index[f[i]->v[j]];
        }
    indexed_normal_flat_.resize(f.size());

    vertex_.resize(f.size() * 3);
    normal_smooth_.resize(f.size() * 3);
    normal_flat_.resize(f.size() * 3);
    for (size_t i = 0; i < f.size(); i++) {
        glm::vec3 normal_flat = glm::normalize(glm::cross(
            f[i]->v[1]->p - f[i]->v[0]->p,
            f[i]->v[2]->p - f[i]->v[1]->p));
        for (int j = 0; j < 3; j++) {
            vertex_[i * 3 + j] = f[i]->v[j]->p;
            normal_smooth_[i * 3 + j] = normals[vertex_index[f[i]->v[j]]];
            normal_flat_[i * 3 + j] = normal_flat;
            index_normal_flat_[i * 3 + j] = static_cast<int>(i);
        }
        indexed_normal_flat_[i] = normal_flat;
    }

    indexed_normal_smooth_ = std::move(normals);

    spdlog::info("Subdivision level {}: {} faces, {} vertices", level_, f.size(), vertex_.size());
}

void LoopSubface::Export(std::string file_name, bool smooth)
{
    file_name = file_name.substr(0, file_name.size() - 4) + "_loop-" + char('0' + level_);
    if (smooth)
        file_name += "-smooth.obj";
    else
        file_name += "-flat.obj";
    std::ofstream ofs(file_name);

    for (auto& v : indexed_vertex_)
        ofs << "v " << v.x << " " << v.y << " " << v.z << std::endl;
    if (smooth) {
        for (auto& n : indexed_normal_smooth_)
            ofs << "vn " << n.x << " " << n.y << " " << n.z << std::endl;
        for (size_t i = 0; i < index_vertex_.size(); i += 3)
            ofs << "f "
                << index_vertex_[i + 0] + 1 << "//" << index_normal_smooth_[i + 0] + 1 << " "
                << index_vertex_[i + 1] + 1 << "//" << index_normal_smooth_[i + 1] + 1 << " "
                << index_vertex_[i + 2] + 1 << "//" << index_normal_smooth_[i + 2] + 1 << std::endl;
    } else {
        for (auto& n : indexed_normal_flat_)
            ofs << "vn " << n.x << " " << n.y << " " << n.z << std::endl;
        for (size_t i = 0; i < index_vertex_.size(); i += 3)
            ofs << "f "
                << index_vertex_[i + 0] + 1 << "//" << index_normal_flat_[i + 0] + 1 << " "
                << index_vertex_[i + 1] + 1 << "//" << index_normal_flat_[i + 1] + 1 << " "
                << index_vertex_[i + 2] + 1 << "//" << index_normal_flat_[i + 2] + 1 << std::endl;
    }

    spdlog::info("Mesh exported: {}", file_name);
}

}
