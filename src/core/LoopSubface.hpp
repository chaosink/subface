#pragma once

#include "Subface.hpp"

#include <string>

namespace subface {

class LoopSubface {
    int level_;

    std::vector<Vertex> vertexes_;
    std::vector<Face> faces_;

    std::vector<glm::vec3> vertex_;
    std::vector<glm::vec3> normal_smooth_;
    std::vector<glm::vec3> normal_flat_;

    std::vector<glm::vec3> indexed_vertex_;
    std::vector<glm::vec3> indexed_normal_smooth_;
    std::vector<glm::vec3> indexed_normal_flat_;
    std::vector<int> index_vertex_;
    std::vector<int> index_normal_smooth_;
    std::vector<int> index_normal_flat_;

    static float Beta(int valence);
    static float LoopGamma(int valence);
    static glm::vec3 WeightOneRing(Vertex* vertex, float beta);
    static glm::vec3 WeightBoundary(Vertex* v, float beta);

public:
    void BuildTopology(const std::vector<glm::vec3>& vertexes, const std::vector<int>& indexes);
    void Subdivide(int level);
    std::vector<glm::vec3>& vertex()
    {
        return vertex_;
    }
    std::vector<glm::vec3>& normal_smooth()
    {
        return normal_smooth_;
    }
    std::vector<glm::vec3>& normal_flat()
    {
        return normal_flat_;
    }
    void Export(std::string file_name, bool smooth);
};

}
