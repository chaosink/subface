#pragma once

class FPS {
	int c_frame_ = 0;
	double time_;
	float fps_ = 0;
public:
	FPS(double time) : time_(time) {}
	float Update(double time);
	void Term();
	float fps() {
		return fps_;
	}
};
