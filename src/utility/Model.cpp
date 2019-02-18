#include "Model.hpp"

#include <cstdio>

#include "glm/gtc/matrix_transform.hpp"
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

Model::Model(GLFWwindow *window, const char *file_name) : window_(window) {
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string err;
	tinyobj::LoadObj(&attrib, &shapes, &materials, &err, file_name);

	for (size_t s = 0; s < shapes.size(); s++)
		for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++)
			n_vertex_ += shapes[s].mesh.num_face_vertices[f];
	vertex_.resize(n_vertex_);
	normal_.resize(n_vertex_);
	uv_.resize(n_vertex_);

	int i = 0;
	for (size_t s = 0; s < shapes.size(); s++) {
		size_t index_offset = 0;
		for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
			size_t fv = shapes[s].mesh.num_face_vertices[f];
			for (size_t v = 0; v < fv; v++) {
				tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
				vertex_[i].x = attrib.vertices[3 * idx.vertex_index + 0];
				vertex_[i].y = attrib.vertices[3 * idx.vertex_index + 1];
				vertex_[i].z = attrib.vertices[3 * idx.vertex_index + 2];
				normal_[i].x = attrib.normals[3 * idx.normal_index + 0];
				normal_[i].y = attrib.normals[3 * idx.normal_index + 1];
				normal_[i].z = attrib.normals[3 * idx.normal_index + 2];
				if(attrib.texcoords.size()) {
					uv_[i].x = attrib.texcoords[2 * idx.texcoord_index + 0];
					uv_[i].y = attrib.texcoords[2 * idx.texcoord_index + 1];
				}
				// Optional: vertex colors
				// tinyobj::real_t red = attrib.colors[3*idx.vertex_index+0];
				// tinyobj::real_t green = attrib.colors[3*idx.vertex_index+1];
				// tinyobj::real_t blue = attrib.colors[3*idx.vertex_index+2];
				i++;
			}
			index_offset += fv;
		}
	}
	printf("Model loaded. Number of faces: %d. Number of vertices: %d\n", n_vertex_ / 3, n_vertex_);
}

glm::mat4 Model::Update(double time) {
	float delta_time = time - time_;
	time_ = time;
	// -y
	if(glfwGetKey(window_, GLFW_KEY_G) == GLFW_PRESS) {
		m_ = glm::rotate(
			glm::mat4(),
			delta_time * turn_speed_,
			glm::vec3(0, -1, 0)
		) * m_;
	}
	// +y
	if(glfwGetKey(window_, GLFW_KEY_J) == GLFW_PRESS) {
		m_ = glm::rotate(
			glm::mat4(),
			delta_time * turn_speed_,
			glm::vec3(0, 1, 0)
		) * m_;
	}
	// +x
	if(glfwGetKey(window_, GLFW_KEY_H) == GLFW_PRESS) {
		m_ = glm::rotate(
			glm::mat4(),
			delta_time * turn_speed_,
			glm::vec3(1, 0, 0)
		) * m_;
	}
	// -x
	if(glfwGetKey(window_, GLFW_KEY_Y) == GLFW_PRESS) {
		m_ = glm::rotate(
			glm::mat4(),
			delta_time * turn_speed_,
			glm::vec3(-1, 0, 0)
		) * m_;
	}
	// +z
	if(glfwGetKey(window_, GLFW_KEY_T) == GLFW_PRESS) {
		m_ = glm::rotate(
			glm::mat4(),
			delta_time * turn_speed_,
			glm::vec3(0, 0, 1)
		) * m_;
	}
	// -z
	if(glfwGetKey(window_, GLFW_KEY_U) == GLFW_PRESS) {
		m_ = glm::rotate(
			glm::mat4(),
			delta_time * turn_speed_,
			glm::vec3(0, 0, -1)
		) * m_;
	}

	if(glfwGetKey(window_, GLFW_KEY_PERIOD) == GLFW_PRESS)
		turn_speed_ *= pow(1.1f, delta_time * 20);
	if(glfwGetKey(window_, GLFW_KEY_COMMA) == GLFW_PRESS)
		turn_speed_ *= pow(0.9f, delta_time * 20);

	if(glfwGetKey(window_, GLFW_KEY_M) == GLFW_PRESS || glfwGetKey(window_, GLFW_KEY_SPACE) == GLFW_PRESS) {
		m_ = glm::mat4();
	}

	return m_;
}
