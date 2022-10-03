#pragma once

#include <string>
#include <vector>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

class Model {
    GLFWwindow* window_;
    glm::mat4 m_ { 1.f };
    double time_ = 0.0;
    float turn_speed_ = 0.5f;

    int n_vertex_ = 0;
    std::vector<glm::vec3> vertex_;
    std::vector<glm::vec3> normal_;
    std::vector<glm::vec2> uv_;

    std::vector<glm::vec3> indexed_vertex_;
    std::vector<uint32_t> index_;

public:
    Model(GLFWwindow* window, const std::string& file_name);
    glm::mat4 Update(double time);

    ~Model() { }
    size_t n_vertex() const
    {
        return vertex_.size();
    }
    const std::vector<glm::vec3>& vertex() const
    {
        return vertex_;
    }
    const std::vector<glm::vec3>& normal() const
    {
        return normal_;
    }
    const std::vector<glm::vec2>& uv() const
    {
        return uv_;
    }
    const std::vector<glm::vec3>& indexed_vertex() const
    {
        return indexed_vertex_;
    }
    const std::vector<uint32_t>& index() const
    {
        return index_;
    }
};
