#include <iostream>
using namespace std;

#include <glm/gtc/matrix_transform.hpp>

#include "OGL.hpp"

#include "Camera.hpp"
#include "Model.hpp"

#include "LoopSubface.hpp"
using namespace subface;

int main(int argc, char* argv[])
{
    bool cmd_mode = false;
    int cmd_level = 0;
    if (argc < 2) {
        printf("Usage: subface model_file [level]\n");
        printf("Cmd mode only with OBJ export and PNG save if level is specified.\n");
        return 0;
    } else if (argc == 3) {
        cmd_mode = true;
        cmd_level = atoi(argv[2]);
    }

    int window_w = 1280;
    int window_h = 720;

    OGL ogl;
    ogl.InitGLFW("Subface", window_w, window_h, cmd_mode);
    ogl.InitGL("shader/vertex.glsl", "shader/fragment.glsl");

    Model model(ogl.window(), argv[1]);

    LoopSubface ls;
    ls.BuildTopology(model.indexed_vertex(), model.index());
    ls.Subdivide(0, false);

    // ogl.Vertex(model.vertex());
    // ogl.Normal(model.normal());
    ogl.Vertex(ls.vertex());
    ogl.Normal(ls.normal_flat());

    Toggle switch_render_mode(ogl.window(), GLFW_KEY_TAB, false);
    int render_mode = 2;
    Toggle enable_smooth_normal(ogl.window(), GLFW_KEY_N, false);
    Toggle enable_cull_face(ogl.window(), GLFW_KEY_C, false);
    Toggle enable_transparent_window(ogl.window(), GLFW_KEY_T, false);
    Toggle export_obj(ogl.window(), GLFW_KEY_O, false);
    Toggle save_png(ogl.window(), GLFW_KEY_F2, false);
    int level = 0, level_old = -1;
    bool flat = false, flat_old = true;

    if (cmd_mode) {
        level = cmd_level;
        enable_smooth_normal.state(true);
    }

    double time = ogl.time();
    Camera camera(ogl.window(), window_w, window_h, time);
    while (ogl.Alive()) {
        time = ogl.time();
        ogl.Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 mvp = camera.Update(time);
        ogl.MVP(mvp);
        ogl.MV(camera.mv());

        // clang-format off
        switch_render_mode.Update([&]() {
            render_mode = (render_mode + 1) % 3;
        });

        enable_smooth_normal.Update([&]() {
            ogl.Normal(ls.normal_smooth());
        }, [&]() {
            ogl.Normal(ls.normal_flat());
        });

        enable_cull_face.Update([&]() {
            glEnable(GL_CULL_FACE);
        }, [&]() {
            glDisable(GL_CULL_FACE);
        });

        enable_transparent_window.Update([&]() {
            glClearColor(0.f, 0.f, 0.f, 0.f);
        }, [&]() {
            glClearColor(0.08f, 0.16f, 0.24f, 1.f);
        });

        auto export_obj_func = [&]() {
            ls.ExportObj(argv[1], enable_smooth_normal.state());
        };
        export_obj.Update(export_obj_func);

        auto save_png_func = [&]() {
            string file_name = argv[1];
            file_name = file_name.substr(0, file_name.size() - 4) + "_loop-" + char('0' + level);
            if (enable_smooth_normal.state())
                file_name += "_smooth.png";
            else
                file_name += "_flat.png";
            ogl.SavePng(file_name);
        };
        save_png.Update(save_png_func);
        // clang-format on

        for (int key = GLFW_KEY_0; key <= GLFW_KEY_9; ++key)
            if (glfwGetKey(ogl.window(), key) == GLFW_PRESS) {
                level = key - GLFW_KEY_0;
                flat = glfwGetKey(ogl.window(), GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS || glfwGetKey(ogl.window(), GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS;
            }

        if (level != level_old || flat != flat_old) {
            level_old = level;
            flat_old = flat;
            // ls.Subdivide(level, flat);
            //ls.Tesselate3(level);
            //ls.Tesselate4(level);
             ls.Tesselate4_1(level);
            ogl.Vertex(ls.vertex());
            if (enable_smooth_normal.state())
                ogl.Normal(ls.normal_smooth());
            else
                ogl.Normal(ls.normal_flat());
        }

        if (render_mode == 0) {
            ogl.Uniform("wireframe", 0);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            ogl.Draw();
        } else if (render_mode == 1) {
            ogl.Uniform("wireframe", 0);
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            ogl.Draw();
        } else if (render_mode == 2) {
            ogl.Uniform("wireframe", 0);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            ogl.Draw();
            ogl.Uniform("wireframe", 1);
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            ogl.Draw();
        }

        ogl.Update();

        if (cmd_mode) {
            export_obj_func();
            save_png_func();
            break;
        }
    }

    return 0;
}
