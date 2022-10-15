#include "OGL.hpp"

#include <fstream>
#include <string>
#include <vector>

#include <spdlog/spdlog.h>
#include <stb_image_write.cpp>

#include "Timer.hpp"

std::vector<std::string> OGL::render_mode_names_ {
    "FacesWireframe",
    "FacesOnly",
    "WireframeOnly",
};

OGL::~OGL()
{
    glDeleteBuffers(1, &position_buffer_);
    glDeleteBuffers(1, &normal_buffer_);
    glDeleteVertexArrays(1, &vertex_array_);
    glDeleteProgram(shader_);

    glfwDestroyWindow(window_);
    glfwTerminate();
}

GLFWwindow* OGL::InitGLFW(std::string window_title, int window_w, int window_h, bool cmd_mode)
{
    std::string func_name = fmt::format("OGL::InitGLFW()");
    Timer timer(func_name);

    window_title_ = window_title;
    window_w_ = window_w;
    window_h_ = window_h;

    if (!glfwInit())
        exit(EXIT_FAILURE);
    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    // glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
    glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);
    if (cmd_mode)
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

    window_ = glfwCreateWindow(window_w_, window_h_, window_title_.c_str(), NULL, NULL);
    if (!window_) {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }
    glfwMakeContextCurrent(window_);

    glewExperimental = true; // Needed for core profile
    if (glewInit() != GLEW_OK) {
        glfwTerminate();
        spdlog::error("{}: Failed to initialize GLEW", func_name);
        exit(EXIT_FAILURE);
    }

    fps_.Init(time());
    camera_ = std::make_unique<Camera>(window_, window_w_, window_h_, time());

    // glfwSwapInterval(1);

    enable_cull_face_ = Toggle(window_, GLFW_KEY_C, false);
    enable_transparent_window_ = Toggle(window_, GLFW_KEY_T, false);
    switch_render_mode_ = Toggle(window_, GLFW_KEY_TAB, false);

    return window_;
}

void OGL::InitGL(const char* vertex_file_path, const char* fragment_file_path, const char* geometry_file_path)
{
    glClearColor(0.08f, 0.16f, 0.24f, 1.f);
    // glClearColor(0.f, 0.f, 0.f, 0.f);
    // glEnable(GL_BLEND);
    // glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    // glEnable(GL_CULL_FACE);

    LoadShader(vertex_file_path, fragment_file_path, geometry_file_path);

    glGenVertexArrays(1, &vertex_array_);
    glBindVertexArray(vertex_array_);
}

GLuint OGL::LoadShaderFromString(const char* vertex_string, const char* fragment_string, const char* geometry_string)
{
    std::string func_name = fmt::format("OGL::LoadShaderFromString()");
    Timer timer(func_name);

    // Create the shaders
    GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
    GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);
    GLuint GeometryShaderID = glCreateShader(GL_GEOMETRY_SHADER);

    GLint Result = GL_FALSE;
    int InfoLogLength;

    // Compile Vertex Shader
    spdlog::info("{}: Compiling shader: vertex", func_name);
    char const* VertexSourcePointer = vertex_string;
    glShaderSource(VertexShaderID, 1, &VertexSourcePointer, NULL);
    glCompileShader(VertexShaderID);
    // Check Vertex Shader
    glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
    glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
    if (InfoLogLength > 0) {
        std::vector<char> VertexShaderErrorMessage(InfoLogLength + 1);
        glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
        spdlog::error("{}: {}", func_name, &VertexShaderErrorMessage[0]);
    }

    // Compile Fragment Shader
    spdlog::info("{}: Compiling shader: fragment", func_name);
    char const* FragmentSourcePointer = fragment_string;
    glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer, NULL);
    glCompileShader(FragmentShaderID);
    // Check Fragment Shader
    glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
    glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
    if (InfoLogLength > 0) {
        std::vector<char> FragmentShaderErrorMessage(InfoLogLength + 1);
        glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
        spdlog::error("{}: {}", func_name, &FragmentShaderErrorMessage[0]);
    }

    if (geometry_string) {
        // Compile Geometry Shader
        spdlog::info("{}: Compiling shader: geometry", func_name);
        char const* GeometrySourcePointer = geometry_string;
        glShaderSource(GeometryShaderID, 1, &GeometrySourcePointer, NULL);
        glCompileShader(GeometryShaderID);
        // Check Geometry Shader
        glGetShaderiv(GeometryShaderID, GL_COMPILE_STATUS, &Result);
        glGetShaderiv(GeometryShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
        if (InfoLogLength > 0) {
            std::vector<char> GeometryShaderErrorMessage(InfoLogLength + 1);
            glGetShaderInfoLog(GeometryShaderID, InfoLogLength, NULL, &GeometryShaderErrorMessage[0]);
            spdlog::error("{}: {}", func_name, &GeometryShaderErrorMessage[0]);
        }
    }

    // Link the program
    spdlog::info("{}: Linking program", func_name);
    GLuint ProgramID = glCreateProgram();
    glAttachShader(ProgramID, VertexShaderID);
    glAttachShader(ProgramID, FragmentShaderID);
    if (geometry_string)
        glAttachShader(ProgramID, GeometryShaderID);
    glLinkProgram(ProgramID);

    // Check the program
    glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
    glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
    if (InfoLogLength > 0) {
        std::vector<char> ProgramErrorMessage(InfoLogLength + 1);
        glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
        spdlog::error("{}: {}", func_name, &ProgramErrorMessage[0]);
    }

    glDeleteShader(VertexShaderID);
    glDeleteShader(FragmentShaderID);
    glDeleteShader(GeometryShaderID);

    return ProgramID;
}

void OGL::LoadShader(const char* vertex_file_path, const char* fragment_file_path, const char* geometry_file_path)
{
    std::string func_name = fmt::format("OGL::LoadShader()");
    Timer timer(func_name);

    // Read the Vertex Shader code from the file
    std::string VertexShaderCode;
    std::ifstream VertexShaderStream(vertex_file_path, std::ios::in);
    if (VertexShaderStream.is_open()) {
        std::string Line = "";
        while (getline(VertexShaderStream, Line))
            VertexShaderCode += "\n" + Line;
        VertexShaderStream.close();
    } else {
        spdlog::error("{}: Impossible to open {}. Are you in the right directory?", func_name, vertex_file_path);
        return;
    }

    // Read the Fragment Shader code from the file
    std::string FragmentShaderCode;
    std::ifstream FragmentShaderStream(fragment_file_path, std::ios::in);
    if (FragmentShaderStream.is_open()) {
        std::string Line = "";
        while (getline(FragmentShaderStream, Line))
            FragmentShaderCode += "\n" + Line;
        FragmentShaderStream.close();
    } else {
        spdlog::error("{}: Impossible to open {}. Are you in the right directory?", func_name, fragment_file_path);
        return;
    }

    // Read the Geometry Shader code from the file
    std::string GeometryShaderCode;
    if (geometry_file_path) {
        std::ifstream GeometryShaderStream(geometry_file_path, std::ios::in);
        if (GeometryShaderStream.is_open()) {
            std::string Line = "";
            while (getline(GeometryShaderStream, Line))
                GeometryShaderCode += "\n" + Line;
            GeometryShaderStream.close();
        } else {
            spdlog::error("{}: Impossible to open {}. Are you in the right directory?", func_name, geometry_file_path);
            return;
        }
    }

    shader_ = LoadShaderFromString(VertexShaderCode.c_str(), FragmentShaderCode.c_str(), geometry_file_path ? GeometryShaderCode.c_str() : NULL);
    glUseProgram(shader_);
    mvp_ = glGetUniformLocation(shader_, "mvp");
    mv_ = glGetUniformLocation(shader_, "mv");
}

void OGL::Position(const std::vector<glm::vec3>& position)
{
    glGenBuffers(1, &position_buffer_);
    glBindBuffer(GL_ARRAY_BUFFER, position_buffer_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * position.size(), position.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        0, // index
        3, // size
        GL_FLOAT, // type
        GL_FALSE, // normalized
        0, // stride
        (void*)0 // pointer
    );
    n_vertex_ = position.size();
}

void OGL::Normal(const std::vector<glm::vec3>& normal)
{
    glGenBuffers(1, &normal_buffer_);
    glBindBuffer(GL_ARRAY_BUFFER, normal_buffer_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * normal.size(), normal.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(
        1, // index
        3, // size
        GL_FLOAT, // type
        GL_FALSE, // normalized
        0, // stride
        (void*)0 // pointer
    );
}

void OGL::MVP(const glm::mat4& mvp) const
{
    glUniformMatrix4fv(mvp_, 1, GL_FALSE, &mvp[0][0]);
}

void OGL::MV(const glm::mat4& mv) const
{
    glUniformMatrix4fv(mv_, 1, GL_FALSE, &mv[0][0]);
}

void OGL::Uniform(const std::string& name, int value) const
{
    GLuint location = glGetUniformLocation(shader_, name.c_str());
    glUniform1i(location, value);
}

bool OGL::Alive() const
{
    return glfwGetKey(window_, GLFW_KEY_ESCAPE) != GLFW_PRESS && !glfwWindowShouldClose(window_);
}

void OGL::Clear() const
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void OGL::Draw() const
{
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(n_vertex_));
}

void OGL::Update(const std::string& program_info)
{
    camera_->Update(time());
    MV(camera_->mv());
    MVP(camera_->mvp());

    enable_transparent_window_.Update();
    if (enable_transparent_window_.state())
        glClearColor(0.f, 0.f, 0.f, 0.f);
    else
        glClearColor(0.08f, 0.16f, 0.24f, 1.f);

    Clear();

    enable_cull_face_.Update();
    enable_cull_face_.state() ? glEnable(GL_CULL_FACE) : glDisable(GL_CULL_FACE);

    switch_render_mode_.Update([&]() {
        render_mode_ = static_cast<ERenderMode>((render_mode_ + 1) % RM_Count);
    });
    if (render_mode_ == RM_FacesWireframe) {
        Uniform("wireframe", 0);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        Draw();
        Uniform("wireframe", 1);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        Draw();
    } else if (render_mode_ == RM_FacesOnly) {
        Uniform("wireframe", 0);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        Draw();
    } else if (render_mode_ == RM_WireframeOnly) {
        Uniform("wireframe", 0);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        Draw();
    }

    glfwSwapBuffers(window_);

    fps_.Update(time());
    std::string title = fmt::format("{}.FPS_{:.2f}.{}",
        window_title_, fps_.fps(), program_info);
    glfwSetWindowTitle(window_, title.c_str());

    glfwPollEvents();
}

void OGL::SavePng(const std::string& file_name) const
{
    std::string func_name = fmt::format("OGL::SavePng(file_name={})", file_name);
    Timer timer(func_name);

    int w, h;
    glfwGetFramebufferSize(window_, &w, &h);
    GLsizei n_channel = 3;
    GLsizei stride = n_channel * w;
    stride += (stride % 4) ? (4 - stride % 4) : 0;
    GLsizei buffer_size = stride * h;
    std::vector<char> buffer(buffer_size);
    glPixelStorei(GL_PACK_ALIGNMENT, 4);
    glReadBuffer(GL_FRONT);
    glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, buffer.data());
    stbi_flip_vertically_on_write(true);
    stbi_write_png(file_name.c_str(), w, h, n_channel, buffer.data(), stride);

    spdlog::info("{}: PNG saved: {}", func_name, file_name);
}
