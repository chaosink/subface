#pragma once

#include "Subface.hpp"

#include <string>

namespace subface {

class LoopSubface {
    int level_;

    std::vector<Vertex> vertexes_;
    std::vector<Face> faces_;

    std::vector<glm::vec3> unindexed_vertexes_;
    std::vector<glm::vec3> unindexed_smooth_normals_;
    std::vector<glm::vec3> unindexed_flat_normals_;

    std::vector<glm::vec3> indexed_vertexes_;
    std::vector<glm::vec3> indexed_smooth_normals_;
    std::vector<glm::vec3> indexed_flat_normals_;

    std::vector<int> vertex_indexes_;
    std::vector<int> smooth_normal_indexes_;
    std::vector<int> flat_normal_indexes_;

    static float Beta(int valence);
    static float LoopGamma(int valence);
    static glm::vec3 WeightOneRing(Vertex* vertex, float beta);
    static glm::vec3 WeightBoundary(Vertex* v, float beta);

public:
    void BuildTopology(const std::vector<glm::vec3>& vertexes, const std::vector<uint32_t>& indexes);
    void Subdivide(int level, bool flat);
    void Tesselate3(int level);
    void Tesselate4(int level);
    void Tesselate4_1(int level);
    std::vector<glm::vec3>& vertex()
    {
        return unindexed_vertexes_;
    }
    std::vector<glm::vec3>& normal_smooth()
    {
        return unindexed_smooth_normals_;
    }
    std::vector<glm::vec3>& normal_flat()
    {
        return unindexed_flat_normals_;
    }
    void ExportObj(std::string file_name, bool smooth);
};

}
