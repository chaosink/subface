#pragma once

#include <functional>

#include <GLFW/glfw3.h>

// Please distinguish between the following 2 cases:
//     1.Call some function when the state CHANGES from on/off to off/on.
//     2.Call some function when the state KEEPS as on/off.
// `Toggle` is designed for case 1.
//     A function passed to `Toggle::Update()` is called only when the state CHANGES, and the changes should
//     be triggered by keypress detected in `Update()`. Changes from setter `state()` won't call functions.
// For case 2, you can check `Toggle::state()` and call some function by yourself.
class Toggle {
    GLFWwindow* window_ = nullptr;
    int key_ = -1;
    bool state_ = false;

    bool pressed_ = false;
    int count_ = 0;
    int jitter_ = 4;

public:
    Toggle() {};

    Toggle(GLFWwindow* window, int key, bool state)
        : window_(window)
        , key_(key)
        , state_(state)
    {
    }

    // Call `Off2On()` when `state_` changes from off(false) to on(true).
    // Call `On2Off()` when `state_` changes from on(true) to off(false).
    bool Update(const std::function<void()>& Off2On, const std::function<void()>& On2Off);
    // Call `F()` when `state_` changes.
    bool Update(const std::function<void()>& F);
    bool Update();
    bool state()
    {
        return state_;
    }
    void state(bool s)
    {
        state_ = s;
    }
};
