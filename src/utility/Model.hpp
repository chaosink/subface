#pragma once

#include <string>
#include <vector>

#include "GLFW/glfw3.h"
#include "glm/glm.hpp"

class Model {
    GLFWwindow* window_;
    glm::mat4 m_;
    double time_;
    float turn_speed_ = 0.5f;

    int n_vertex_ = 0;
    std::vector<glm::vec3> vertex_;
    std::vector<glm::vec3> normal_;
    std::vector<glm::vec2> uv_;

    std::vector<glm::vec3> indexed_vertex_;
    std::vector<uint32_t> index_;

public:
    Model(GLFWwindow* window, const std::string& file_name);
    ~Model() { }
    size_t n_vertex()
    {
        return vertex_.size();
    }
    std::vector<glm::vec3>& vertex()
    {
        return vertex_;
    }
    std::vector<glm::vec3>& normal()
    {
        return normal_;
    }
    std::vector<glm::vec2>& uv()
    {
        return uv_;
    }
    std::vector<glm::vec3>& indexed_vertex()
    {
        return indexed_vertex_;
    }
    std::vector<uint32_t>& index()
    {
        return index_;
    }
    glm::mat4 Update(double time);
};
