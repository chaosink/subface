#include "FPS.hpp"

float FPS::Update(double time)
{
    if (c_frame_++ % 10 == 0) {
        fps_ = static_cast<float>(10 / (time - time_));
        time_ = time;
    }
    return fps_;
}
