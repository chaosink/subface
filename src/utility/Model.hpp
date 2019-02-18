#pragma once

#include "GLFW/glfw3.h"
#include "glm/glm.hpp"
#include <vector>

class Model {
	GLFWwindow *window_;
	glm::mat4 m_;
	double time_;
	float turn_speed_ = 0.5f;

	int n_vertex_ = 0;
	std::vector<glm::vec3> vertex_;
	std::vector<glm::vec3> normal_;
	std::vector<glm::vec2> uv_;

public:
	Model(GLFWwindow *window, const char *file_name);
	~Model() {}
	int n_vertex() {
		return vertex_.size();
	}
	std::vector<glm::vec3>& vertex() {
		return vertex_;
	}
	std::vector<glm::vec3>& normal() {
		return normal_;
	}
	std::vector<glm::vec2>& uv() {
		return uv_;
	}
	glm::mat4 Update(double time);
};
