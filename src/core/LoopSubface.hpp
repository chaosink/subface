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

    static float Beta(int valence);
    static float LoopGamma(int valence);
    static glm::vec3 WeightOneRing(Vertex* vertex, float beta);
    static glm::vec3 WeightBoundary(Vertex* v, float beta);
    static void BuildTopology(const std::vector<glm::vec3>& positions, const std::vector<uint32_t>& indexes,
        std::vector<Vertex>& vertexes, std::vector<Face>& faces);

    void ComputeNormalsAndPositions(const std::vector<Vertex*>& vertexes, const std::vector<Face*>& faces);

public:
    void BuildTopology(const std::vector<glm::vec3>& vertexes, const std::vector<uint32_t>& indexes);
    void Subdivide(int level, bool flat);
    void Tesselate3(int level);
    void Tesselate4(int level);
    void Tesselate4_1(int level);
    void Decimate(int level, bool sloppy);

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
    void ExportObj(std::string file_name, bool smooth) const;
};

}
