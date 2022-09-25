#pragma once

#include <chrono>
#include <string>

class Timer {
    enum State {
        Stopped,
        Timing,
        Paused,
    };

    std::string name_;
    State state_;
    std::chrono::time_point<std::chrono::steady_clock> start_, end_;
    std::chrono::duration<double> duration_;

public:
    Timer(std::string name);
    ~Timer();

    void Reset();
    void Pause();
    void Continue();
    void Snapshot(const std::string& name);
    void Stop();
};
