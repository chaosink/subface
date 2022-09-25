#include "OGL.hpp"

#include <fstream>
#include <string>
#include <vector>

OGL::~OGL()
{
    glDeleteBuffers(1, &vertex_buffer_);
    glDeleteBuffers(1, &normal_buffer_);
    glDeleteVertexArrays(1, &vertex_array_);
    glDeleteProgram(shader_);
    glfwDestroyWindow(window_);
    glfwTerminate();
}

GLFWwindow* OGL::InitGLFW(std::string window_title, int window_w, int window_h)
{
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

    window_ = glfwCreateWindow(window_w_, window_h_, window_title_.c_str(), NULL, NULL);
    if (!window_) {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }
    glfwMakeContextCurrent(window_);

    glewExperimental = true; // Needed for core profile
    if (glewInit() != GLEW_OK) {
        glfwTerminate();
        fprintf(stderr, "Failed to initialize GLEW\n");
        exit(EXIT_FAILURE);
    }

    fps_.Init(time());

    // glfwSwapInterval(1);
    return window_;
}

void OGL::InitGL(const char* vertex_file_path, const char* fragment_file_path, const char* geometry_file_path)
{
    // glClearColor(0.08f, 0.16f, 0.24f, 1.f);
    // glClearColor(0.f, 0.f, 0.f, 0.f);
    // glEnable(GL_BLEND);
    // glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);

    LoadShader(vertex_file_path, fragment_file_path, geometry_file_path);

    glGenVertexArrays(1, &vertex_array_);
    glBindVertexArray(vertex_array_);
}

GLuint OGL::LoadShaderFromString(const char* vertex_string, const char* fragment_string, const char* geometry_string)
{
    // Create the shaders
    GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
    GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);
    GLuint GeometryShaderID = glCreateShader(GL_GEOMETRY_SHADER);

    GLint Result = GL_FALSE;
    int InfoLogLength;

    // Compile Vertex Shader
    printf("Compiling shader: %s\n", "vertex");
    char const* VertexSourcePointer = vertex_string;
    glShaderSource(VertexShaderID, 1, &VertexSourcePointer, NULL);
    glCompileShader(VertexShaderID);
    // Check Vertex Shader
    glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
    glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
    if (InfoLogLength > 0) {
        std::vector<char> VertexShaderErrorMessage(InfoLogLength + 1);
        glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
        printf("%s\n", &VertexShaderErrorMessage[0]);
    }

    // Compile Fragment Shader
    printf("Compiling shader: %s\n", "fragment");
    char const* FragmentSourcePointer = fragment_string;
    glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer, NULL);
    glCompileShader(FragmentShaderID);
    // Check Fragment Shader
    glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
    glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
    if (InfoLogLength > 0) {
        std::vector<char> FragmentShaderErrorMessage(InfoLogLength + 1);
        glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
        printf("%s\n", &FragmentShaderErrorMessage[0]);
    }

    if (geometry_string) {
        // Compile Geometry Shader
        printf("Compiling shader: %s\n", "geometry");
        char const* GeometrySourcePointer = geometry_string;
        glShaderSource(GeometryShaderID, 1, &GeometrySourcePointer, NULL);
        glCompileShader(GeometryShaderID);
        // Check Geometry Shader
        glGetShaderiv(GeometryShaderID, GL_COMPILE_STATUS, &Result);
        glGetShaderiv(GeometryShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
        if (InfoLogLength > 0) {
            std::vector<char> GeometryShaderErrorMessage(InfoLogLength + 1);
            glGetShaderInfoLog(GeometryShaderID, InfoLogLength, NULL, &GeometryShaderErrorMessage[0]);
            printf("%s\n", &GeometryShaderErrorMessage[0]);
        }
    }

    // Link the program
    printf("Linking program\n");
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
        printf("%s\n", &ProgramErrorMessage[0]);
    }

    glDeleteShader(VertexShaderID);
    glDeleteShader(FragmentShaderID);
    glDeleteShader(GeometryShaderID);

    return ProgramID;
}

void OGL::LoadShader(const char* vertex_file_path, const char* fragment_file_path, const char* geometry_file_path)
{
    // Read the Vertex Shader code from the file
    std::string VertexShaderCode;
    std::ifstream VertexShaderStream(vertex_file_path, std::ios::in);
    if (VertexShaderStream.is_open()) {
        std::string Line = "";
        while (getline(VertexShaderStream, Line))
            VertexShaderCode += "\n" + Line;
        VertexShaderStream.close();
    } else {
        printf("Impossible to open %s. Are you in the right directory?\n", vertex_file_path);
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
        printf("Impossible to open %s. Are you in the right directory?\n", fragment_file_path);
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
            printf("Impossible to open %s. Are you in the right directory?\n", geometry_file_path);
            return;
        }
    }

    shader_ = LoadShaderFromString(VertexShaderCode.c_str(), FragmentShaderCode.c_str(), geometry_file_path ? GeometryShaderCode.c_str() : NULL);
    glUseProgram(shader_);
    mvp_ = glGetUniformLocation(shader_, "mvp");
    mv_ = glGetUniformLocation(shader_, "mv");
}

void OGL::Vertex(const std::vector<glm::vec3>& vertex)
{
    glGenBuffers(1, &vertex_buffer_);
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * vertex.size(), vertex.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        0, // index
        3, // size
        GL_FLOAT, // type
        GL_FALSE, // normalized
        0, // stride
        (void*)0 // pointer
    );
    n_vertex_ = vertex.size();
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

void OGL::MVP(const glm::mat4& mvp)
{
    glUniformMatrix4fv(mvp_, 1, GL_FALSE, &mvp[0][0]);
}

void OGL::MV(const glm::mat4& mv)
{
    glUniformMatrix4fv(mv_, 1, GL_FALSE, &mv[0][0]);
}

void OGL::Uniform(const std::string& name, int value)
{
    GLuint location = glGetUniformLocation(shader_, name.c_str());
    glUniform1i(location, value);
}

bool OGL::Alive()
{
    return glfwGetKey(window_, GLFW_KEY_ESCAPE) != GLFW_PRESS && !glfwWindowShouldClose(window_);
}

void OGL::Clear(GLenum bit)
{
    glClear(bit);
}

void OGL::Draw()
{
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(n_vertex_));
}

#include <spdlog/fmt/fmt.h>
void OGL::Update()
{
    glfwSwapBuffers(window_);
    glfwPollEvents();

    fps_.Update(time());
    std::string title = fmt::format("{} {:6.2f} FPS", window_title_, fps_.fps());
    glfwSetWindowTitle(window_, title.c_str());
}
