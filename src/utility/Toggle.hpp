#pragma once

#include <GLFW/glfw3.h>
#include <functional>

class Toggle {
    GLFWwindow* window_;
    int key_;
    bool pressed_ = false;
    bool state_;
    int count_ = -1;
    int jitter_ = 4;

public:
    Toggle(GLFWwindow* window, int key, bool state)
        : window_(window)
        , key_(key)
        , state_(state)
    {
    }
    bool Update(std::function<void()> Off2On, std::function<void()> On2Off);
    bool Update(std::function<void()> F);
    bool Update();
    bool state()
    {
        return state_;
    }
};
