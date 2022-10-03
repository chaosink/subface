#pragma once

#include <string>
#include <vector>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include "FPS.hpp"

class OGL {
    int window_w_ = -1, window_h_ = -1;
    GLFWwindow* window_ = nullptr;
    std::string window_title_;

    FPS fps_;

    GLuint shader_ = -1;
    GLuint mvp_ = -1, mv_ = -1;
    GLuint vertex_array_ = -1, vertex_buffer_ = -1, normal_buffer_ = -1;
    size_t n_vertex_ = -1;

    GLuint LoadShaderFromString(const char* vertex_string, const char* fragment_string, const char* geometry_string = nullptr);
    void LoadShader(const char* vertex_file_path, const char* fragment_file_path, const char* geometry_file_path = nullptr);

public:
    ~OGL();
    GLFWwindow* InitGLFW(std::string window_title, int window_w, int window_h, bool cmd_mode);
    void InitGL(const char* vertex_file_path, const char* fragment_file_path, const char* geometry_file_path = nullptr);
    void Vertex(const std::vector<glm::vec3>& vertex);
    void Normal(const std::vector<glm::vec3>& normal);
    void MVP(const glm::mat4& mvp);
    void MV(const glm::mat4& mv);
    void Uniform(const std::string& name, int value);
    bool Alive();
    void Clear(GLenum bit);
    void Draw();
    void Update(const std::string& info);
    void SavePng(const std::string& file_name);
    double time()
    {
        return glfwGetTime();
    }
    GLFWwindow* window()
    {
        return window_;
    }
    void EnableCullFace(bool cull)
    {
        cull ? glEnable(GL_CULL_FACE) : glDisable(GL_CULL_FACE);
    }
    void EnableTransparentWindow(bool transparent)
    {
        if (transparent)
            glClearColor(0.f, 0.f, 0.f, 0.f);
        else
            glClearColor(0.08f, 0.16f, 0.24f, 1.f);
    }
};
