#pragma once

#include "Subface.hpp"

#include <string>

namespace subface {

class LoopSubface {
    int level_ = 0;

    std::vector<glm::vec3> origin_positions_;
    std::vector<uint32_t> origin_indexes_;

    std::vector<Vertex> vertexes_;
    std::vector<Face> faces_;

    std::vector<glm::vec3> unindexed_positions_;
    std::vector<glm::vec3> unindexed_smooth_normals_;
    std::vector<glm::vec3> unindexed_flat_normals_;

    std::vector<glm::vec3> indexed_positions_;
    std::vector<glm::vec3> indexed_smooth_normals_;
    std::vector<glm::vec3> indexed_flat_normals_;

    std::vector<int> vertex_indexes_;
    std::vector<int> smooth_normal_indexes_;
    std::vector<int> flat_normal_indexes_;

    // Only for non-boundary vertexes.
    static float Beta(int valence);
    // Only for non-boundary vertexes.
    static float LoopGamma(int valence);
    static glm::vec3 WeightOneRing(Vertex* vertex, float beta);
    // Only for boundary vertexes.
    static glm::vec3 WeightBoundary(Vertex* v, float beta);
    static void BuildTopology(const std::vector<glm::vec3>& positions, const std::vector<uint32_t>& indexes,
        std::vector<Vertex>& vertexes, std::vector<Face>& faces);

    void ComputeNormalsAndPositions(const std::vector<Vertex*>& vertexes, const std::vector<Face*>& faces);

public:
    void BuildTopology(const std::vector<glm::vec3>& vertexes, const std::vector<uint32_t>& indexes);
    // Same as Tessellate4(int level) if `flat==true`.
    // `compute_limit` matters only when `flat==false`.
    void Subdivide(int level, bool flat, bool compute_limit);
    // Same as Subdivide(int level, bool flat=true).
    void Tessellate4(int level);
    // Another 1-to-4 triangle tessellation pattern than `Tessellate4()`.
    void Tessellate4_1(int level);
    // 1-to-3 triangle tessellation by connecting the center to each vertex.
    void Tessellate3(int level);
    // Use the implementations from https://github.com/zeux/meshoptimizer
    // Reduces the number of triangles in the mesh.
    // if `sloppy==false`:
    //     Attempte to preserve mesh appearance as much as possible.
    // else:
    //     Sacrifice mesh appearance for simplification performance.
    void MeshoptDecimate(int level, bool sloppy);

    const std::vector<glm::vec3>& vertex() const
    {
        return unindexed_positions_;
    }
    const std::vector<glm::vec3>& normal_smooth() const
    {
        return unindexed_smooth_normals_;
    }
    const std::vector<glm::vec3>& normal_flat() const
    {
        return unindexed_flat_normals_;
    }
    void ExportObj(const std::string& file_name, bool smooth) const;
};

}
