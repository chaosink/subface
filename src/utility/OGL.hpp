#pragma once

#include <vector>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

class OGL {
	int window_w_, window_h_;
	GLFWwindow *window_;

	GLuint shader_;
	GLuint mvp_, mv_;
	GLuint vertex_array_, vertex_buffer_, normal_buffer_;
	int n_vertex_;

	GLuint LoadShaderFromString(const char *vertex_string, const char *fragment_string, const char *geometry_string = nullptr);
	void LoadShader(const char *vertex_file_path, const char *fragment_file_path, const char *geometry_file_path = nullptr);
public:
	~OGL();
	GLFWwindow* InitGLFW(const char *title, int window_w, int window_h);
	void InitGL(const char *vertex_file_path, const char *fragment_file_path, const char *geometry_file_path = nullptr);
	void Vertex(std::vector<glm::vec3> &vertex);
	void Normal(std::vector<glm::vec3> &normal);
	void MVP(glm::mat4 mvp);
	void MV(glm::mat4 mv);
	bool Alive();
	void Clear(GLenum bit);
	void Update();
	double time() {
		return glfwGetTime();
	}
	GLFWwindow* window() {
		return window_;
	}
};
