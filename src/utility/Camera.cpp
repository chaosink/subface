#include "Camera.hpp"

#include <cstdio>

#include <glm/gtc/matrix_transform.hpp>

static double scoll = 0;
static void ScrollCallback(GLFWwindow* /*window*/, double /*x*/, double y)
{
    scoll = y;
}

void PrintMat(const glm::mat4& m, const char* indent, const char* name)
{
    printf("\n");
    printf("%s", indent);
    if (name)
        printf("%s = ", name);
    printf("glm::mat4(\n");
    printf("%s	%f, %f, %f, %f,\n", indent, m[0][0], m[0][1], m[0][2], m[0][3]);
    printf("%s	%f, %f, %f, %f,\n", indent, m[1][0], m[1][1], m[1][2], m[1][3]);
    printf("%s	%f, %f, %f, %f,\n", indent, m[2][0], m[2][1], m[2][2], m[2][3]);
    printf("%s	%f, %f, %f, %f\n", indent, m[3][0], m[3][1], m[3][2], m[3][3]);
    printf("%s);\n", indent);
}

void PrintVec(const glm::vec3& v, const char* indent, const char* name)
{
    printf("\n");
    printf("%s", indent);
    if (name)
        printf("%s = ", name);
    printf("glm::vec3(%f, %f, %f);\n", v.x, v.y, v.z);
}

Camera::Camera(GLFWwindow* window, int window_w, int window_h, double time)
    : window_(window)
    , window_w_(window_w)
    , window_h_(window_h)
    , time_(time)
{
    glfwSetScrollCallback(window, ScrollCallback);
    // glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    // glfwGetCursorPos(window_, &x_, &y_);
}

void Camera::Update(double time)
{
    update_fix_.Update([&]() {
        mv_fix_ = mv_;
        mvp_fix_ = mvp_;
    });

    fix_.Update();
    if (fix_.state()) {
        mv_ = mv_fix_;
        mvp_ = mvp_fix_;
        return;
    }

    float delta_time = static_cast<float>(time - time_);
    time_ = time;

    if (glfwGetMouseButton(window_, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
        if (!mouse_button_right_pressed_) {
            glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            glfwGetCursorPos(window_, &x_, &y_);
            mouse_button_right_pressed_ = true;
        } else {
            double x, y;
            glfwGetCursorPos(window_, &x, &y);
            angle_horizontal_ += mouse_turn_factor_ * float(x_ - x);
            angle_vertical_ += mouse_turn_factor_ * float(y_ - y);
            x_ = x;
            y_ = y;
        }
    } else {
        glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        mouse_button_right_pressed_ = false;
    }

    float turn_speed = turn_speed_;
    float move_speed = move_speed_;
    if (glfwGetKey(window_, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS
        || glfwGetKey(window_, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS) {
        turn_speed *= 0.1f;
        move_speed *= 0.1f;
    }

    // turn right
    if (glfwGetKey(window_, GLFW_KEY_L) == GLFW_PRESS) {
        angle_horizontal_ -= delta_time * turn_speed;
    }
    // turn left
    if (glfwGetKey(window_, GLFW_KEY_J) == GLFW_PRESS) {
        angle_horizontal_ += delta_time * turn_speed;
    }
    // turn  up
    if (glfwGetKey(window_, GLFW_KEY_I) == GLFW_PRESS) {
        angle_vertical_ += delta_time * turn_speed;
    }
    // turn down
    if (glfwGetKey(window_, GLFW_KEY_K) == GLFW_PRESS) {
        angle_vertical_ -= delta_time * turn_speed;
    }

    // Direction: Spherical coordinates to Cartesian coordinates conversion
    glm::vec3 direction(
        cos(angle_vertical_) * sin(angle_horizontal_),
        sin(angle_vertical_),
        cos(angle_vertical_) * cos(angle_horizontal_));
    // Right vector
    glm::vec3 right(
        sin(angle_horizontal_ - PI / 2.0f),
        0.f,
        cos(angle_horizontal_ - PI / 2.0f));
    // Up vector
    glm::vec3 up = glm::cross(right, direction);

    if (!mouse_button_right_pressed_ && glfwGetMouseButton(window_, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        if (!mouse_button_left_pressed_) {
            double x, y;
            glfwGetCursorPos(window_, &x, &y);
            x_ = x;
            y_ = y;
            mouse_button_left_pressed_ = true;
        } else {
            double x, y;
            glfwGetCursorPos(window_, &x, &y);
            if (x != x_ || y != y_) {
                glm::vec3 offset = right * static_cast<float>(x - x_) - up * static_cast<float>(y - y_);
                glm::vec3 axis = glm::cross(offset, direction); // No need to normalize `axis`. `glm::rotate()` will do that.
                m_ = glm::rotate(glm::mat4(1.f), glm::length(offset) / 100.f, axis) * m_;
                x_ = x;
                y_ = y;
            }
        }
    } else {
        mouse_button_left_pressed_ = false;
    }

    // move forward
    if (glfwGetKey(window_, GLFW_KEY_W) == GLFW_PRESS || glfwGetKey(window_, GLFW_KEY_UP) == GLFW_PRESS)
        position_ += delta_time * move_speed * direction;
    // move backward
    if (glfwGetKey(window_, GLFW_KEY_S) == GLFW_PRESS || glfwGetKey(window_, GLFW_KEY_DOWN) == GLFW_PRESS)
        position_ -= delta_time * move_speed * direction;
    // move right
    if (glfwGetKey(window_, GLFW_KEY_D) == GLFW_PRESS || glfwGetKey(window_, GLFW_KEY_RIGHT) == GLFW_PRESS)
        position_ += delta_time * move_speed * right;
    // move left
    if (glfwGetKey(window_, GLFW_KEY_A) == GLFW_PRESS || glfwGetKey(window_, GLFW_KEY_LEFT) == GLFW_PRESS)
        position_ -= delta_time * move_speed * right;
    // move up
    if (glfwGetKey(window_, GLFW_KEY_E) == GLFW_PRESS || glfwGetKey(window_, GLFW_KEY_PAGE_UP) == GLFW_PRESS)
        position_ += delta_time * move_speed * up;
    // move down
    if (glfwGetKey(window_, GLFW_KEY_Q) == GLFW_PRESS || glfwGetKey(window_, GLFW_KEY_PAGE_DOWN) == GLFW_PRESS)
        position_ -= delta_time * move_speed * up;

    if (glfwGetKey(window_, GLFW_KEY_EQUAL) == GLFW_PRESS)
        move_speed_ *= 1.1f;
    if (glfwGetKey(window_, GLFW_KEY_MINUS) == GLFW_PRESS)
        move_speed_ *= 0.9f;
    if (glfwGetKey(window_, GLFW_KEY_RIGHT_BRACKET) == GLFW_PRESS)
        turn_speed_ *= 1.1f;
    if (glfwGetKey(window_, GLFW_KEY_LEFT_BRACKET) == GLFW_PRESS)
        turn_speed_ *= 0.9f;

    if (glfwGetKey(window_, GLFW_KEY_SPACE) == GLFW_PRESS) {
        m_ = m_init_;
        position_ = position_init_;
        angle_horizontal_ = angle_horizontal_init_;
        angle_vertical_ = angle_vertical_init_;
        fov_ = fov_init_;
    }
    print_vp_.Update([this] { // Call this lambda when toggle `print_vp_` turns on.
        PrintMat(m_, "\t\t", "m_");
        PrintMat(v_, "\t\t", "v_");
        PrintMat(p_, "\t\t", "p_");
        PrintMat(mv_, "\t\t", "mv_");
        PrintMat(mvp_, "\t\t", "mvp_");
    });
    fov_ += static_cast<float>(delta_time * scroll_speed_ * scoll);
    scoll = 0;

    // Camera matrix
    v_ = glm::lookAt(
        position_, // Camera is here
        position_ + direction, // and looks here: at the same position_, plus "direction"
        up); // Head is up (set to 0,-1,0 to look upside-down)
    // Projection matrix: 45° Field of View, 4:3 ratio, display range : 0.1 unit <-> 100 units
    p_ = glm::perspective(fov_, float(window_w_) / window_h_, 0.01f, 100.f);

    mv_ = v_ * m_;
    mvp_ = p_ * mv_;
}
