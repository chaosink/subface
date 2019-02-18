#include <iostream>
using namespace std;

#include <glm/gtc/matrix_transform.hpp>

#include "OGL.hpp"
#include "Camera.hpp"
#include "FPS.hpp"
#include "Model.hpp"

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
	ogl.Vertex(model.vertex());
	ogl.Normal(model.normal());

	Toggle render_mode(ogl.window(), GLFW_KEY_TAB, false);

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

		render_mode.Update([&]() {
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			glDisable(GL_CULL_FACE);
		}, [&]() {
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			glEnable(GL_CULL_FACE);
		});

		ogl.Update();
		fps.Update(time);
	}
	fps.Term();

	return 0;
}
