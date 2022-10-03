#include "Model.hpp"

#include <glm/gtc/matrix_transform.hpp>
#define TINYOBJLOADER_IMPLEMENTATION
#include <spdlog/spdlog.h>
#include <tiny_obj_loader.h>

#include "Timer.hpp"

Model::Model(GLFWwindow* window, const std::string& file_name)
    : window_(window)
{
    std::string func_name = fmt::format("Model::Model()");
    Timer timer(func_name);

    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string err;
    tinyobj::LoadObj(&attrib, &shapes, &materials, &err, file_name.c_str());

    for (size_t s = 0; s < shapes.size(); s++)
        for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++)
            n_vertex_ += shapes[s].mesh.num_face_vertices[f];
    vertex_.resize(n_vertex_);
    normal_.resize(n_vertex_);
    uv_.resize(n_vertex_);
    index_.resize(n_vertex_);

    int i = 0;
    for (size_t s = 0; s < shapes.size(); s++) {
        size_t index_offset = 0;
        for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
            size_t fv = shapes[s].mesh.num_face_vertices[f];
            for (size_t v = 0; v < fv; v++) {
                tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
                size_t vi = idx.vertex_index, ni = idx.normal_index, ti = idx.texcoord_index;
                vertex_[i].x = attrib.vertices[3 * vi + 0];
                vertex_[i].y = attrib.vertices[3 * vi + 1];
                vertex_[i].z = attrib.vertices[3 * vi + 2];
                normal_[i].x = attrib.normals[3 * ni + 0];
                normal_[i].y = attrib.normals[3 * ni + 1];
                normal_[i].z = attrib.normals[3 * ni + 2];
                if (attrib.texcoords.size()) {
                    uv_[i].x = attrib.texcoords[2 * ti + 0];
                    uv_[i].y = attrib.texcoords[2 * ti + 1];
                }
                // Optional: vertex colors
                // tinyobj::real_t red = attrib.colors[3*idx.vertex_index+0];
                // tinyobj::real_t green = attrib.colors[3*idx.vertex_index+1];
                // tinyobj::real_t blue = attrib.colors[3*idx.vertex_index+2];

                index_[i] = idx.vertex_index;
                i++;
            }
            index_offset += fv;
        }
    }

    indexed_vertex_.resize(attrib.vertices.size() / 3);
    for (size_t i = 0; i < attrib.vertices.size() / 3; ++i) {
        indexed_vertex_[i] = glm::vec3(
            attrib.vertices[i * 3 + 0],
            attrib.vertices[i * 3 + 1],
            attrib.vertices[i * 3 + 2]);
    }

    spdlog::info("{}: Model {} loaded. {} triangls, {} vertices.", func_name, file_name, n_vertex_ / 3, indexed_vertex_.size());
}

glm::mat4 Model::Update(double time)
{
    float delta_time = static_cast<float>(time - time_);
    time_ = time;
    // -y
    if (glfwGetKey(window_, GLFW_KEY_G) == GLFW_PRESS) {
        m_ = glm::rotate(
                 glm::mat4(),
                 delta_time * turn_speed_,
                 glm::vec3(0, -1, 0))
            * m_;
    }
    // +y
    if (glfwGetKey(window_, GLFW_KEY_J) == GLFW_PRESS) {
        m_ = glm::rotate(
                 glm::mat4(),
                 delta_time * turn_speed_,
                 glm::vec3(0, 1, 0))
            * m_;
    }
    // +x
    if (glfwGetKey(window_, GLFW_KEY_H) == GLFW_PRESS) {
        m_ = glm::rotate(
                 glm::mat4(),
                 delta_time * turn_speed_,
                 glm::vec3(1, 0, 0))
            * m_;
    }
    // -x
    if (glfwGetKey(window_, GLFW_KEY_Y) == GLFW_PRESS) {
        m_ = glm::rotate(
                 glm::mat4(),
                 delta_time * turn_speed_,
                 glm::vec3(-1, 0, 0))
            * m_;
    }
    // +z
    if (glfwGetKey(window_, GLFW_KEY_T) == GLFW_PRESS) {
        m_ = glm::rotate(
                 glm::mat4(),
                 delta_time * turn_speed_,
                 glm::vec3(0, 0, 1))
            * m_;
    }
    // -z
    if (glfwGetKey(window_, GLFW_KEY_U) == GLFW_PRESS) {
        m_ = glm::rotate(
                 glm::mat4(),
                 delta_time * turn_speed_,
                 glm::vec3(0, 0, -1))
            * m_;
    }

    if (glfwGetKey(window_, GLFW_KEY_PERIOD) == GLFW_PRESS)
        turn_speed_ *= pow(1.1f, delta_time * 20);
    if (glfwGetKey(window_, GLFW_KEY_COMMA) == GLFW_PRESS)
        turn_speed_ *= pow(0.9f, delta_time * 20);

    if (glfwGetKey(window_, GLFW_KEY_M) == GLFW_PRESS || glfwGetKey(window_, GLFW_KEY_SPACE) == GLFW_PRESS) {
        m_ = glm::mat4();
    }

    return m_;
}
