#pragma once

class FPS {
    int c_frame_ = 0;
    double time_ = 0;
    float fps_ = 0;

public:
    float Update(double time);

    FPS() { }
    FPS(double time)
    {
        Init(time);
    }
    void Init(double time)
    {
        time_ = time;
    }
    float fps() const
    {
        return fps_;
    }
};
