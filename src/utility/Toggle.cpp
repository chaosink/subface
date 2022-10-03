#include "Toggle.hpp"

bool Toggle::Update(const std::function<void()> &Off2On, const std::function<void()> &On2Off)
{
    if (count_-- > 0)
        return state_;
    if (glfwGetKey(window_, key_) == GLFW_PRESS) {
        if (!pressed_) {
            state_ = !state_;
            if (state_)
                Off2On();
            // Call Off2On() when state_ changes from off(false) to on(true).
            else
                On2Off();
            // Call On2Off() when state_ changes from on(true) to off(false).
            pressed_ = true;
            count_ = jitter_;
        }
    } else {
        if (pressed_) {
            pressed_ = false;
            count_ = jitter_;
        }
    }
    return state_;
}

bool Toggle::Update(const std::function<void()> &F)
{
    if (count_-- > 0)
        return state_;
    if (glfwGetKey(window_, key_) == GLFW_PRESS) {
        if (!pressed_) {
            state_ = !state_;
            F(); // Call F() when state_ changes.
            pressed_ = true;
            count_ = jitter_;
        }
    } else {
        if (pressed_) {
            pressed_ = false;
            count_ = jitter_;
        }
    }
    return state_;
}

bool Toggle::Update()
{
    if (count_-- > 0)
        return state_;
    if (glfwGetKey(window_, key_) == GLFW_PRESS) {
        if (!pressed_) {
            state_ = !state_;
            pressed_ = true;
            count_ = jitter_;
        }
    } else {
        if (pressed_) {
            pressed_ = false;
            count_ = jitter_;
        }
    }
    return state_;
}
