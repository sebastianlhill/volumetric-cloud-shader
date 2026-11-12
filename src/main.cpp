#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <cstdlib>
#include <iostream>
#include <string>

#include "shader.h"
#include "TileableVolumeNoise.h"
#include "cloudbake.h"
#include "volumeio.h"
#include <glm/vec3.hpp>

static void error_callback(int error, const char* description) {
    std::cerr << "glfw error " << error << " : " << description << '\n';
}

static void framebuffer_size_callback(GLFWwindow*, int width, int height) {
    glViewport(0, 0, width, height);
}

// State shared with the key callback so it can hot-reload the shader.
struct ReloadState {
    std::string frag_path;
    GLuint      program = 0;
    GLint       u_time  = -1;
    GLint       u_res   = -1;
    GLint       u_cloud_shape   = -1;
    GLint       u_cloud_erosion = -1;
};

struct CloudTextures {
    GLuint shape   = 0;
    GLuint erosion = 0;
    bool baked     = false;
};
// Loads (or reloads) the program into `s`. On failure leaves `s` untouched.
static bool loadProgramInto(ReloadState& s, const std::string& vert_path) {
    try {
        GLuint np = loadShaderProgram(vert_path, s.frag_path);
        if (s.program) glDeleteProgram(s.program);
        s.program = np;
        s.u_time  = glGetUniformLocation(np, "u_time");
        s.u_res   = glGetUniformLocation(np, "u_resolution");
        s.u_cloud_shape = glGetUniformLocation(np, "uCloudShape");
        s.u_cloud_erosion = glGetUniformLocation(np, "uCloudErosion");
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Shader load failed, keeping previous program:\n"
                  << e.what() << '\n';
        return false;
    }
}

static void key_callback(GLFWwindow* window, int key, int /*scancode*/, int action, int /*mods*/) {
    if (action != GLFW_PRESS) return;

    if (key == GLFW_KEY_ESCAPE) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
        return;
    }

    if (key == GLFW_KEY_R) {
        auto* s = static_cast<ReloadState*>(glfwGetWindowUserPointer(window));
        if (s && loadProgramInto(*s, "shaders/fullscreen.vert"))
            std::cout << "Reloaded " << s->frag_path << '\n';
    }
}


// Accepts:
//   program                 -> shaders/fbm.frag  (default)
//   program fbm             -> shaders/fbm.frag  (adds .frag, prefixes shaders/)
//   program warp.frag       -> shaders/warp.frag
//   program shaders/x.frag  -> used verbatim (contains a path separator)
//   program -f <name>       -> flag form, same rules as above
//   program --frag <name>   -> flag form
static std::string resolveFragPath(int argc, char** argv) {
    const std::string default_frag = "shaders/clouds.frag";

    std::string arg;
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "-f" || a == "--frag") {
            if (i + 1 >= argc) {
                std::cerr << a << " requires an argument\n";
                std::exit(2);
            }
            arg = argv[++i];
            break;
        } else if (!a.empty() && a[0] != '-') {
            arg = a;
            break;
        }
    }

    if (arg.empty()) return default_frag;

    const bool has_sep = arg.find('/') != std::string::npos
                      || arg.find('\\') != std::string::npos;
    if (has_sep) return arg;                     // treat as a full path

    if (arg.size() < 5 || arg.substr(arg.size() - 5) != ".frag")
        arg += ".frag";
    return "shaders/" + arg;
}

static bool createVolumeTextureFromFile(const char* path, GLuint& outTex){
  int N = 0, C = 0;
  std::vector<float> data;

  if (!loadVolume3D(path, N, C, data)) {
        std::cerr << "loadVolume3D failed: " << path << '\n';
        return false;
    }
std::printf("%s: N=%d C=%d floats=%zu\n", path, N, C, data.size());
    if (C != 4) {
        std::cerr << "Expected 4 channels, got " << C << " in " << path << '\n';
        return false;
    }
    glGenTextures(1, &outTex);
    glBindTexture(GL_TEXTURE_3D, outTex);
    glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA16F,
                 N, N, N, 0,
                 GL_RGBA, GL_FLOAT, data.data());
    
  GLenum err = glGetError();
  if (err) std::fprintf(stderr, "glTexImage3D error 0x%x\n", err);

  GLint tw = 0, th = 0, td = 0;
  glGetTexLevelParameteriv(GL_TEXTURE_3D, 0, GL_TEXTURE_WIDTH,  &tw);
  glGetTexLevelParameteriv(GL_TEXTURE_3D, 0, GL_TEXTURE_HEIGHT, &th);
  glGetTexLevelParameteriv(GL_TEXTURE_3D, 0, GL_TEXTURE_DEPTH,  &td);
  std::printf("  texture dims: %dx%dx%d\n", tw, th, td);

    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);
    glGenerateMipmap(GL_TEXTURE_3D);
    glBindTexture(GL_TEXTURE_3D, 0);

    std::printf("Loaded %s (%d^3, %d channels)\n", path, N, C);
    
    std::vector<float> readback(size_t(N)*N*N*4);
glBindTexture(GL_TEXTURE_3D, outTex);
glGetTexImage(GL_TEXTURE_3D, 0, GL_RGBA, GL_FLOAT, readback.data());

int nonzero = 0;
float mn = 1e9f, mx = -1e9f;
for (float v : readback) { if (v > 0.001f) nonzero++; mn = std::min(mn,v); mx = std::max(mx,v); }
std::printf("  readback: nonzero=%d/%zu  min=%.3f max=%.3f\n",
            nonzero, readback.size(), mn, mx);
    return true;
}

int main(int argc, char** argv) {
    glfwSetErrorCallback(error_callback);
    if (!glfwInit()) return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif

    const int win_w = 1280, win_h = 720;
    GLFWwindow* window = glfwCreateWindow(win_w, win_h, "shader testbench", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetKeyCallback(window, key_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialise GLAD\n";
        return -1;
    }

    // Fullscreen triangle (clips to the whole viewport).
    const float tri_verts[] = {
        -1.0f, -1.0f,
         3.0f, -1.0f,
        -1.0f,  3.0f,
    };

    GLuint vao = 0, vbo = 0;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(tri_verts), tri_verts, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
   
    // Resolve the fragment shader and load the program.
    ReloadState state;
    CloudTextures ct; 
    createVolumeTextureFromFile("data/cloudshape.vol", ct.shape);
    createVolumeTextureFromFile("data/clouderosion.vol", ct.erosion);

    state.frag_path = resolveFragPath(argc, argv);
    std::cout << "Loading fragment shader: " << state.frag_path << '\n';

    const std::string vert_path = "shaders/fullscreen.vert";
    if (!loadProgramInto(state, vert_path)) {
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    // Expose state to the key callback for hot reload.
    glfwSetWindowUserPointer(window, &state);

    const float start_time = (float)glfwGetTime();

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        int fbw = 0, fbh = 0;
        glfwGetFramebufferSize(window, &fbw, &fbh);
        glViewport(0, 0, fbw, fbh);
        glClear(GL_COLOR_BUFFER_BIT);

        const float t = (float)glfwGetTime() - start_time;

        glUseProgram(state.program);
        glUniform1f(state.u_time, t);
        glUniform2f(state.u_res, (float)fbw, (float)fbh);

        if (state.u_cloud_shape >= 0 && state.u_cloud_erosion >= 0) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_3D, ct.shape);
            glUniform1i(state.u_cloud_shape, 0);
            
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_3D, ct.erosion);
            glUniform1i(state.u_cloud_erosion, 1);
        }
    

    // draw loop
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glfwSwapBuffers(window);
    }
    glDeleteTextures(1, &ct.shape);
    glDeleteTextures(1, &ct.erosion);
    glDeleteProgram(state.program);
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

