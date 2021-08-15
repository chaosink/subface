#include "Timer.hpp"

#include <spdlog/spdlog.h>

Timer::Timer(std::string name) : name_(name) {
	Reset();
}

Timer::~Timer() {
	Stop();
}

void Timer::Reset() {
	start_ = end_ = std::chrono::steady_clock::now();
	duration_ = {0};
	state_ = Timing;
}

void Timer::Pause() {
	if(state_ == Timing) {
		end_ = std::chrono::steady_clock::now();
		duration_ += end_ - start_;
		state_ = Paused;
	} else {
		spdlog::warn("Wrong call of Timer::Pause()");
	}
}

void Timer::Continue() {
	if(state_ == Paused) {
		start_ = end_ = std::chrono::steady_clock::now();
		state_ = Timing;
	} else {
		spdlog::warn("Wrong call of Timer::Pause()");
	}
}

void Timer::Snapshot(const std::string &name) {
	if(state_ == Timing) {
		end_ = std::chrono::steady_clock::now();
		duration_ += end_ - start_;
	}
	spdlog::info("Time of {} at {}: {}", name_, name, duration_.count());
}

void Timer::Stop() {
	if(state_ != Stopped) {
		if(state_ == Timing) {
			end_ = std::chrono::steady_clock::now();
			duration_ += end_ - start_;
		}
		state_ = Stopped;

		spdlog::info("Time of {}: {}", name_, duration_.count());
	}
}
