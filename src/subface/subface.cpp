#include <iostream>
using namespace std;

#include <spdlog/fmt/fmt.h>

#include <glm/gtc/matrix_transform.hpp>

#include "OGL.hpp"

#include "Camera.hpp"
#include "Model.hpp"

#include "LoopSubface.hpp"
using namespace subface;

struct ProcessingMethod {
    std::string name;
    std::function<void(LoopSubface& ls, int level)> process;
};
std::vector<ProcessingMethod> processing_methods = {
    { "SubdivideSmooth",
        [](LoopSubface& ls, int level) {
            ls.Subdivide(level, false, true);
        } }, // Key 1
    { "SubdivideSmoothNoLimit",
        [](LoopSubface& ls, int level) {
            ls.Subdivide(level, false, false);
        } }, // Key 2
    { "SubdivideFlat",
        [](LoopSubface& ls, int level) {
            ls.Subdivide(level, true, false);
        } }, // Key 3
    { "Tessellate4",
        [](LoopSubface& ls, int level) {
            ls.Tessellate4(level);
        } }, // Key 4
    { "Tessellate4_1",
        [](LoopSubface& ls, int level) {
            ls.Tessellate4_1(level);
        } }, // Key 5
    { "Tessellate3",
        [](LoopSubface& ls, int level) {
            ls.Tessellate3(level);
        } }, // Key 6
    { "MeshoptDecimate",
        [](LoopSubface& ls, int level) {
            ls.MeshoptDecimate(level, false);
        } }, // Key 7
    { "MeshoptDecimateSloppy",
        [](LoopSubface& ls, int level) {
            ls.MeshoptDecimate(level, true);
        } }, // Key 8
};

enum RenderMode {
    RenderMode_FacesWireframe,
    RenderMode_FacesOnly,
    RenderMode_WireframeOnly,
    RenderMode_Count,
};
std::vector<std::string> render_mode_names {
    "FacesWireframe",
    "FacesOnly",
    "WireframeOnly",
};

int main(int argc, char* argv[])
{
    bool cmd_mode = false;
    int cmd_method = 0, cmd_level = 0;
    if (argc < 2) {
        std::cout << "Usage: subface model_file [processing_method_id level]" << std::endl;
        std::cout << "Cmd mode with OBJ export and PNG save is used if processing_method_id and level is specified." << std::endl;
        std::cout << "All processing methods:" << std::endl;
        for (int i = 0; i < processing_methods.size(); ++i)
            std::cout << fmt::format("    {}.{}", i + 1, processing_methods[i].name) << std::endl;
        return 0;
    } else if (argc == 4) {
        cmd_mode = true;
        cmd_method = atoi(argv[2]) - 1;
        cmd_level = atoi(argv[3]);
    }
    const std::string file_path = argv[1];

    int window_w = 1280;
    int window_h = 720;

    OGL ogl;
    ogl.InitGLFW("Subface", window_w, window_h, cmd_mode);
    ogl.InitGL("shader/vertex.glsl", "shader/fragment.glsl");

    Model model(ogl.window(), file_path);

    LoopSubface ls;
    ls.BuildTopology(model.indexed_vertex(), model.index());
    ls.Subdivide(0, false, true);

    // ogl.Vertex(model.vertex());
    // ogl.Normal(model.normal());
    ogl.Vertex(ls.vertex());
    ogl.Normal(ls.normal_flat());

    Toggle switch_render_mode(ogl.window(), GLFW_KEY_TAB, false);
    RenderMode render_mode = RenderMode_FacesWireframe;
    Toggle enable_smooth_normal(ogl.window(), GLFW_KEY_N, false);
    Toggle enable_cull_face(ogl.window(), GLFW_KEY_C, false);
    Toggle enable_transparent_window(ogl.window(), GLFW_KEY_T, false);
    Toggle export_obj(ogl.window(), GLFW_KEY_O, false);
    Toggle save_png(ogl.window(), GLFW_KEY_F2, false);
    int method = 0, method_old = -1;
    int level = 0, level_old = -1;

    double time = ogl.time();
    Camera camera(ogl.window(), window_w, window_h, time);

    if (cmd_mode) {
        method = cmd_method;
        level = cmd_level;
        // enable_smooth_normal.state(true);
        camera.Fix(true);
    }

    while (ogl.Alive()) {
        time = ogl.time();
        ogl.Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        camera.Update(time);
        ogl.MV(camera.mv());
        ogl.MVP(camera.mvp());

        // clang-format off
        switch_render_mode.Update([&]() {
            render_mode = static_cast<RenderMode>((render_mode + 1) % RenderMode_Count);
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

        auto get_file_name = [&]() {
            return fmt::format("{}.{}.{}",
                file_path.substr(0, file_path.find_last_of('.')),
                level == 0 ? "origin" : processing_methods[method].name + ".level_" + char('0' + level),
                enable_smooth_normal.state() ? "smooth" : "flat"
            );
        };

        auto export_obj_func = [&]() {
            ls.ExportObj(get_file_name() + ".obj", enable_smooth_normal.state());
        };
        export_obj.Update(export_obj_func);

        auto save_png_func = [&]() {
            ogl.SavePng(get_file_name() + ".png");
        };
        save_png.Update(save_png_func);
        // clang-format on

        for (int key = GLFW_KEY_0; key <= GLFW_KEY_9; ++key)
            if (glfwGetKey(ogl.window(), key) == GLFW_PRESS)
                if (glfwGetKey(ogl.window(), GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS || glfwGetKey(ogl.window(), GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS) {
                    if (GLFW_KEY_1 <= key && key <= GLFW_KEY_0 + processing_methods.size())
                        method = (key - GLFW_KEY_1) % static_cast<int>(processing_methods.size());
                } else
                    level = key - GLFW_KEY_0;

        if (level != level_old || method != method_old) {
            level_old = level;
            method_old = method;
            processing_methods[method].process(ls, level);
            ogl.Vertex(ls.vertex());
            if (enable_smooth_normal.state())
                ogl.Normal(ls.normal_smooth());
            else
                ogl.Normal(ls.normal_flat());
        }

        if (render_mode == RenderMode_FacesWireframe) {
            ogl.Uniform("wireframe", 0);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            ogl.Draw();
            ogl.Uniform("wireframe", 1);
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            ogl.Draw();
        } else if (render_mode == RenderMode_FacesOnly) {
            ogl.Uniform("wireframe", 0);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            ogl.Draw();
        } else if (render_mode == RenderMode_WireframeOnly) {
            ogl.Uniform("wireframe", 0);
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            ogl.Draw();
        }

        std::string title = fmt::format("{}(level={}) | {} | Cull {}",
            processing_methods[method].name, level, render_mode_names[render_mode], enable_cull_face.state());
        ogl.Update(title);

        if (cmd_mode) {
            // export_obj_func();
            save_png_func();
            break;
        }
    }

    return 0;
}
