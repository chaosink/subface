#include <iostream>

#include <argparse/argparse.hpp>
#include <spdlog/spdlog.h>

#include "Model.hpp"
#include "OGL.hpp"

#include "Subface.hpp"
using namespace subface;

int main(int argc, char* argv[])
{
    argparse::ArgumentParser program("subface", "v0.1.0");
    // Program description.
    std::string description = "Process geometries with one of the following methods:\n";
    for (int i = 0; i < Subface::PM_Count; ++i)
        description += fmt::format("    {}.{}\n", i + 1, Subface::GetProcessingMethod(static_cast<Subface::EProcessingMethod>(i)).name);
    program.add_description(description);
    // Positional arguments.
    program.add_argument("OBJ_file_path")
        .help("OBJ file path");
    // Optional arguments giving flags.
    program.add_argument("--cmd", "-c")
        .help("run in command line mode")
        .default_value(false)
        .implicit_value(true);
    program.add_argument("--export_obj", "-e")
        .help("export OBJ in command line mode")
        .default_value(false)
        .implicit_value(true);
    program.add_argument("--save_png", "-s")
        .help("save PNG in command line mode")
        .default_value(false)
        .implicit_value(true);
    program.add_argument("--smooth", "-n")
        .help("use smooth normal")
        .default_value(false)
        .implicit_value(true);
    program.add_argument("--fix_camera", "-f")
        .help("fix camera")
        .default_value(false)
        .implicit_value(true);
    program.add_argument("--cull", "-u")
        .help("enable face culling")
        .default_value(false)
        .implicit_value(true);
    program.add_argument("--transparent", "-t")
        .help("enable transparent window")
        .default_value(false)
        .implicit_value(true);
    // Optional arguments giving values.
    program.add_argument("--render", "-r")
        .help("render mode ID")
        .default_value(0)
        .scan<'i', int>();
    program.add_argument("--method", "-m")
        .help("processing method ID")
        .default_value(1)
        .scan<'i', int>();
    program.add_argument("--level", "-l")
        .help("processing level")
        .default_value(0)
        .scan<'i', int>();
    // Parse arguments.
    try {
        program.parse_args(argc, argv);
    } catch (const std::exception& err) {
        std::cerr << err.what() << std::endl
                  << std::endl;
        std::cerr << program;
        std::exit(1);
    }
    // Get parsed arguments.
    const std::string file_path = program.get<std::string>("OBJ_file_path");
    bool cmd_mode = program.get<bool>("--cmd");
    bool cmd_export_obj = program.get<bool>("--export_obj");
    bool cmd_save_png = program.get<bool>("--save_png");
    bool arg_smooth_normal = program.get<bool>("--smooth");
    bool fix_camera = program.get<bool>("--fix_camera");
    bool cull_face = program.get<bool>("--cull");
    bool transparent_window = program.get<bool>("--transparent");
    OGL::ERenderMode render_mode = static_cast<OGL::ERenderMode>(program.get<int>("--render") % OGL::RM_Count);
    Subface::EProcessingMethod method = static_cast<Subface::EProcessingMethod>((program.get<int>("--method") - 1 + Subface::PM_Count) % Subface::PM_Count);
    int level = program.get<int>("--level") % 10;

    int window_w = 1280;
    int window_h = 720;

    OGL ogl;
    ogl.InitGLFW("Subface", window_w, window_h, cmd_mode);
    ogl.InitGL("shader/vertex.glsl", "shader/fragment.glsl");
    ogl.EnableCullFace(cull_face);
    ogl.EnableTransparentWindow(transparent_window);
    ogl.RenderMode(render_mode);
    ogl.FixCamera(fix_camera);

    Toggle use_smooth_normal(ogl.window(), GLFW_KEY_N, arg_smooth_normal);
    Toggle decimate_one_less_face(ogl.window(), GLFW_KEY_COMMA, false);
    Toggle decimate_one_more_face(ogl.window(), GLFW_KEY_PERIOD, false);
    Toggle export_obj(ogl.window(), GLFW_KEY_O, false);
    Toggle save_png(ogl.window(), GLFW_KEY_F2, false);

    Model model(ogl.window(), file_path);

    Subface sf;
    sf.BuildTopology(model.indexed_vertex(), model.index());

    auto process = [&](Subface::EProcessingMethod method, int level) {
        Subface::GetProcessingMethod(method).process(sf, level);
        ogl.Position(sf.Position());
        if (use_smooth_normal.state())
            ogl.Normal(sf.NormalSmooth());
        else
            ogl.Normal(sf.NormalFlat());

        // ogl.Vertex(model.vertex());
        // ogl.Normal(model.normal());
    };
    process(method, level);

    Subface::EProcessingMethod method_old = method;
    int level_old = level;
    while (ogl.Alive()) {
        // clang-format off
        use_smooth_normal.Update([&]() {
            ogl.Normal(sf.NormalSmooth());
        }, [&]() {
            ogl.Normal(sf.NormalFlat());
        });
        // clang-format on

        for (int key = GLFW_KEY_0; key <= GLFW_KEY_9; ++key)
            if (glfwGetKey(ogl.window(), key) == GLFW_PRESS) {
                if (glfwGetKey(ogl.window(), GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS || glfwGetKey(ogl.window(), GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS) {
                    // Larger complexity: subdivision and tessellation.
                    if (GLFW_KEY_1 <= key && key < GLFW_KEY_1 + Subface::PM_Decimate_Start)
                        method = static_cast<Subface::EProcessingMethod>(key - GLFW_KEY_1);
                } else if (glfwGetKey(ogl.window(), GLFW_KEY_LEFT_ALT) == GLFW_PRESS || glfwGetKey(ogl.window(), GLFW_KEY_RIGHT_ALT) == GLFW_PRESS) {
                    // Smaller complexity: decimation.
                    if (GLFW_KEY_1 <= key && key < GLFW_KEY_1 + Subface::PM_Decimate_End - Subface::PM_Decimate_Start)
                        method = static_cast<Subface::EProcessingMethod>(key - GLFW_KEY_1 + Subface::PM_Decimate_Start);
                } else {
                    level = key - GLFW_KEY_0;
                }
            }

        if (level != level_old || method != method_old) {
            level_old = level;
            method_old = method;
            process(method, level);
        }
        if (Subface::PM_Decimate_Start <= method && method <= Subface::PM_Decimate_End) { // Decimation methods.
            decimate_one_less_face.Update([&]() {
                process(method, -1); // `level == -1` means "decimate one less face".
            });
            decimate_one_more_face.Update([&]() {
                process(method, -2); // `level == -2` means "decimate one more face".
            });
            if (glfwGetKey(ogl.window(), GLFW_KEY_LEFT_ALT) == GLFW_PRESS || glfwGetKey(ogl.window(), GLFW_KEY_RIGHT_ALT) == GLFW_PRESS) {
                if (glfwGetKey(ogl.window(), GLFW_KEY_COMMA) == GLFW_PRESS)
                    process(method, -1); // `level == -1` means "decimate one less face
                if (glfwGetKey(ogl.window(), GLFW_KEY_PERIOD) == GLFW_PRESS)
                    process(method, -2); // `level == -2` means "decimate one more face
            }
        }

        auto get_processing_info = [&]() {
            return fmt::format("{}.Normal_{}",
                level == 0 ? "origin" : fmt::format("{}(level={})", Subface::GetProcessingMethod(method).name, level),
                use_smooth_normal.state() ? "smooth" : "flat");
        };
        auto get_rendering_info = [&]() {
            return fmt::format("{}.Cull_{}",
                ogl.RenderModeName(), ogl.EnableCullFace());
        };

        auto export_obj_func = [&]() {
            std::string file_name = fmt::format("{}.{}.obj",
                file_path.substr(0, file_path.find_last_of('.')),
                get_processing_info());
            sf.ExportObj(file_name, use_smooth_normal.state());
        };
        export_obj.Update(export_obj_func);

        auto save_png_func = [&]() {
            std::string file_name = fmt::format("{}.{}.{}.png",
                file_path.substr(0, file_path.find_last_of('.')),
                get_processing_info(), get_rendering_info());
            ogl.SavePng(file_name);
        };
        save_png.Update(save_png_func);

        std::string program_info = fmt::format("{}.{}",
            get_processing_info(), get_rendering_info());
        ogl.Update(program_info);

        if (cmd_mode) {
            if (cmd_export_obj)
                export_obj_func();
            if (cmd_save_png)
                save_png_func();
            break;
        }
    }

    return 0;
}
