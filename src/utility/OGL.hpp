#pragma once

#include <string>
#include <vector>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include "Camera.hpp"
#include "FPS.hpp"
#include "Toggle.hpp"

class OGL {
public:
    enum ERenderMode {
        RM_FacesWireframe,
        RM_FacesOnly,
        RM_WireframeOnly,
        RM_Count,
    };
    static std::vector<std::string> render_mode_names_;

private:
    int window_w_ = -1, window_h_ = -1;
    GLFWwindow* window_ = nullptr;
    std::string window_title_;

    FPS fps_;
    std::unique_ptr<Camera> camera_;

    GLuint shader_ = -1;
    GLuint mvp_ = -1, mv_ = -1;
    GLuint vertex_array_ = -1, position_buffer_ = -1, normal_buffer_ = -1;
    size_t n_vertex_ = -1;

    Toggle enable_cull_face_;
    Toggle enable_transparent_window_;
    Toggle switch_render_mode_;
    ERenderMode render_mode_ = RM_FacesWireframe;

    static GLuint LoadShaderFromString(const char* vertex_string, const char* fragment_string, const char* geometry_string = nullptr);
    void LoadShader(const char* vertex_file_path, const char* fragment_file_path, const char* geometry_file_path = nullptr);

public:
    ~OGL();
    GLFWwindow* InitGLFW(std::string window_title, int window_w, int window_h, bool cmd_mode);
    void InitGL(const char* vertex_file_path, const char* fragment_file_path, const char* geometry_file_path = nullptr);
    void Position(const std::vector<glm::vec3>& vertex);
    void Normal(const std::vector<glm::vec3>& normal);
    void MVP(const glm::mat4& mvp) const;
    void MV(const glm::mat4& mv) const;
    void Uniform(const std::string& name, int value) const;
    bool Alive() const;
    void Clear() const;
    void Draw() const;
    void Update(const std::string& program_info);
    void SavePng(const std::string& file_name) const;

    double time() const
    {
        return glfwGetTime();
    }
    GLFWwindow* window() const
    {
        return window_;
    }
    void EnableCullFace(bool cull)
    {
        enable_cull_face_.state(cull);
    }
    void EnableTransparentWindow(bool transparent)
    {
        enable_transparent_window_.state(transparent);
    }
    void RenderMode(ERenderMode render_mode)
    {
        render_mode_ = render_mode;
    }
    void FixCamera(bool fix_camera)
    {
        camera_->Fix(fix_camera);
    }
};
