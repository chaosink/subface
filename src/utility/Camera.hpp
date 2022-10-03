#pragma once

#include "Toggle.hpp"

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

void PrintMat(const glm::mat4& m, const char* indent = "", const char* name = NULL);
void PrintVec(const glm::vec3& v, const char* indent = "", const char* name = NULL);

class Camera {
    const double PI = 3.14159265358979323846;

    GLFWwindow* window_;
    int window_w_, window_h_;

    const glm::mat4 m_init_ = glm::scale(glm::mat4(1.f), glm::vec3(0.2f));
    glm::mat4 m_ = m_init_, v_ { 1.f }, p_ { 1.f }, mv_ { 1.f }, mvp_ { 1.f };
    glm::mat4 mv_fix_ { 0.182916, 0.041963, -0.069143, 0.000000, 0.000000, 0.170976, 0.103765, 0.000000, 0.080881, -0.094902, 0.156371, 0.000000, 0.008600, 0.010363, -0.546909, 1.000000 };
    glm::mat4 mvp_fix_ { 0.248399, 0.101308, 0.069157, 0.069143, 0.000000, 0.412772, -0.103786, -0.103765, 0.109836, -0.229113, -0.156402, -0.156371, 0.011679, 0.025018, 0.527017, 0.546909 };

    const glm::vec3 position_init_ = glm::vec3(0.f, 0.f, 1.f);
    const float angle_horizontal_init_ = static_cast<float>(PI);
    const float angle_vertical_init_ = 0.f;
    const float fov_init_ = static_cast<float>(PI / 4.f);

    glm::vec3 position_ = position_init_;
    float angle_horizontal_ = angle_horizontal_init_;
    float angle_vertical_ = angle_vertical_init_;
    float fov_ = fov_init_;

    float move_speed_ = 2.0f;
    float turn_speed_ = 0.5f;
    float mouse_turn_factor_ = 0.002f;
    float scroll_speed_ = 2.f;

    double time_ = 0;
    double x_ = 0, y_ = 0;

    Toggle fix_ = Toggle(window_, GLFW_KEY_F, false);
    Toggle print_vp_ = Toggle(window_, GLFW_KEY_P, false);
    Toggle update_fix_ = Toggle(window_, GLFW_KEY_R, false);
    bool mouse_button_left_pressed_ = false;
    bool mouse_button_right_pressed_ = false;

public:
    Camera(GLFWwindow* window, int window_w, int window_h, double time);
    void Update(double time);

    const glm::mat4& mv()
    {
        return mv_;
    }
    const glm::mat4& mvp()
    {
        return mvp_;
    }
    void Fix(bool fix)
    {
        fix_.state(fix);
    }
};
