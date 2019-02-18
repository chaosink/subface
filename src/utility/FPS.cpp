#include "FPS.hpp"

#include <cstdio>

float FPS::Update(double time) {
	if(c_frame_++ % 10 == 0) {
		fps_ = 10 / (time - time_);
		time_ = time;
		printf("\rFPS: %f", fps_);
		fflush(stdout);
	}
	return fps_;
}

void FPS::Term() {
	printf("\n");
}
