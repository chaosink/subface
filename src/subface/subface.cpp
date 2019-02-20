#include <iostream>
using namespace std;

#include <glm/gtc/matrix_transform.hpp>

#include "OGL.hpp"
#include "Camera.hpp"
#include "FPS.hpp"
#include "Model.hpp"
#include "LoopSubface.hpp"
using namespace subface;

int main(int argc, char *argv[]) {
	if(argc < 2) {
		printf("Usage: subface model_file\n");
		return 0;
	}

	int window_w = 1280;
	int window_h = 720;

	OGL ogl;
	ogl.InitGLFW("Subface", window_w, window_h);
	ogl.InitGL("shader/vertex.glsl", "shader/fragment.glsl");

	Model model(ogl.window(), argv[1]);

	LoopSubface ls;
	ls.BuildTopology(model.indexed_vertex(), model.index());
	ls.Subdivide(0);

	// ogl.Vertex(model.vertex());
	// ogl.Normal(model.normal());
	ogl.Vertex(ls.vertex());
	ogl.Normal(ls.normal_flat());

	Toggle switch_render_mode(ogl.window(), GLFW_KEY_TAB, false);
	int render_mode = 2;
	Toggle enable_smooth_normal(ogl.window(), GLFW_KEY_N, false);
	Toggle enable_cull_face(ogl.window(), GLFW_KEY_C, false);
	Toggle export_mesh(ogl.window(), GLFW_KEY_O, false);
	int level = 0, level_old = 0;

	double time = ogl.time();
	Camera camera(ogl.window(), window_w, window_h, time);
	FPS fps(time);
	while(ogl.Alive()) {
		time = ogl.time();
		ogl.Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glm::mat4 m = glm::scale(glm::mat4(), glm::vec3(0.2f));
		glm::mat4 vp = camera.Update(time);
		glm::mat4 mvp = vp * m;
		ogl.MVP(mvp);
		glm::mat4 v = camera.v();
		glm::mat4 mv = v * m;
		ogl.MV(mv);

		switch_render_mode.Update([&]() {
			render_mode = (render_mode + 1) % 3;
		});

		enable_smooth_normal.Update([&]() {
			ogl.Normal(ls.normal_smooth());
		}, [&]() {
			ogl.Normal(ls.normal_flat());
		});

		enable_cull_face.Update([&]() {
			glDisable(GL_CULL_FACE);
		}, [&]() {
			glEnable(GL_CULL_FACE);
		});

		export_mesh.Update([&]() {
			ls.Export(argv[1], enable_smooth_normal.state());
		});

		for(int key = GLFW_KEY_0; key <= GLFW_KEY_9; ++key)
			if(glfwGetKey(ogl.window(), key) == GLFW_PRESS)
				level = key - GLFW_KEY_0;
		if(level != level_old) {
			level_old = level;
			ls.Subdivide(level);
			ogl.Vertex(ls.vertex());
			if(enable_smooth_normal.state())
				ogl.Normal(ls.normal_smooth());
			else
				ogl.Normal(ls.normal_flat());
		}

		if(render_mode == 0) {
			ogl.Uniform("wireframe", 0);
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			ogl.Draw();
		} else if(render_mode == 1) {
			ogl.Uniform("wireframe", 0);
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			ogl.Draw();
		} else if(render_mode == 2) {
			ogl.Uniform("wireframe", 0);
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			ogl.Draw();
			ogl.Uniform("wireframe", 1);
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			ogl.Draw();
		}

		ogl.Update();
		fps.Update(time);
	}
	fps.Term();

	return 0;
}
