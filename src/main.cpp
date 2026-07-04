#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>

#include "Shader.h"
#include "Camera.h"

// ── Window config ─────────────────────────────────────────────────────────────
constexpr int   WIN_W  = 1280;
constexpr int   WIN_H  = 720;
constexpr char  WIN_T[] = "Voxel Engine";

// ── Globals (used by GLFW callbacks) ─────────────────────────────────────────
static Camera camera;
static bool   mouseDown = false;
static float  lastX     = WIN_W / 2.0f;
static float  lastY     = WIN_H / 2.0f;
static int    winW      = WIN_W;
static int    winH      = WIN_H;

// ── Callbacks ────────────────────────────────────────────────────────────────
static void cb_framebuffer(GLFWwindow*, int w, int h) {
    winW = w; winH = h;
    glViewport(0, 0, w, h);
}

static void cb_key(GLFWwindow* win, int key, int, int action, int) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(win, GLFW_TRUE);
}

static void cb_mouse_button(GLFWwindow*, int btn, int action, int) {
    if (btn == GLFW_MOUSE_BUTTON_LEFT)
        mouseDown = (action == GLFW_PRESS);
}

static void cb_cursor(GLFWwindow*, double xpos, double ypos) {
    const float dx = static_cast<float>(xpos) - lastX;
    const float dy = static_cast<float>(ypos) - lastY;
    lastX = static_cast<float>(xpos);
    lastY = static_cast<float>(ypos);

    if (mouseDown)
        camera.orbit(dx * 0.3f, -dy * 0.3f);
}

static void cb_scroll(GLFWwindow*, double, double dy) {
    camera.zoom(static_cast<float>(dy) * 0.4f);
}

// ── Cube geometry ─────────────────────────────────────────────────────────────
// 36 vertices, each: position(3) + normal(3)
// clang-format off
static constexpr float CUBE_VERTS[] = {
    // Back face
    -0.5f,-0.5f,-0.5f,  0.0f, 0.0f,-1.0f,
     0.5f,-0.5f,-0.5f,  0.0f, 0.0f,-1.0f,
     0.5f, 0.5f,-0.5f,  0.0f, 0.0f,-1.0f,
     0.5f, 0.5f,-0.5f,  0.0f, 0.0f,-1.0f,
    -0.5f, 0.5f,-0.5f,  0.0f, 0.0f,-1.0f,
    -0.5f,-0.5f,-0.5f,  0.0f, 0.0f,-1.0f,
    // Front face
    -0.5f,-0.5f, 0.5f,  0.0f, 0.0f, 1.0f,
     0.5f,-0.5f, 0.5f,  0.0f, 0.0f, 1.0f,
     0.5f, 0.5f, 0.5f,  0.0f, 0.0f, 1.0f,
     0.5f, 0.5f, 0.5f,  0.0f, 0.0f, 1.0f,
    -0.5f, 0.5f, 0.5f,  0.0f, 0.0f, 1.0f,
    -0.5f,-0.5f, 0.5f,  0.0f, 0.0f, 1.0f,
    // Left face
    -0.5f, 0.5f, 0.5f, -1.0f, 0.0f, 0.0f,
    -0.5f, 0.5f,-0.5f, -1.0f, 0.0f, 0.0f,
    -0.5f,-0.5f,-0.5f, -1.0f, 0.0f, 0.0f,
    -0.5f,-0.5f,-0.5f, -1.0f, 0.0f, 0.0f,
    -0.5f,-0.5f, 0.5f, -1.0f, 0.0f, 0.0f,
    -0.5f, 0.5f, 0.5f, -1.0f, 0.0f, 0.0f,
    // Right face
     0.5f, 0.5f, 0.5f,  1.0f, 0.0f, 0.0f,
     0.5f, 0.5f,-0.5f,  1.0f, 0.0f, 0.0f,
     0.5f,-0.5f,-0.5f,  1.0f, 0.0f, 0.0f,
     0.5f,-0.5f,-0.5f,  1.0f, 0.0f, 0.0f,
     0.5f,-0.5f, 0.5f,  1.0f, 0.0f, 0.0f,
     0.5f, 0.5f, 0.5f,  1.0f, 0.0f, 0.0f,
    // Bottom face
    -0.5f,-0.5f,-0.5f,  0.0f,-1.0f, 0.0f,
     0.5f,-0.5f,-0.5f,  0.0f,-1.0f, 0.0f,
     0.5f,-0.5f, 0.5f,  0.0f,-1.0f, 0.0f,
     0.5f,-0.5f, 0.5f,  0.0f,-1.0f, 0.0f,
    -0.5f,-0.5f, 0.5f,  0.0f,-1.0f, 0.0f,
    -0.5f,-0.5f,-0.5f,  0.0f,-1.0f, 0.0f,
    // Top face
    -0.5f, 0.5f,-0.5f,  0.0f, 1.0f, 0.0f,
     0.5f, 0.5f,-0.5f,  0.0f, 1.0f, 0.0f,
     0.5f, 0.5f, 0.5f,  0.0f, 1.0f, 0.0f,
     0.5f, 0.5f, 0.5f,  0.0f, 1.0f, 0.0f,
    -0.5f, 0.5f, 0.5f,  0.0f, 1.0f, 0.0f,
    -0.5f, 0.5f,-0.5f,  0.0f, 1.0f, 0.0f,
};
// clang-format on

// ── main ─────────────────────────────────────────────────────────────────────
int main() {
    // Init GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4); // MSAA 4x

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(WIN_W, WIN_H, WIN_T, nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, cb_framebuffer);
    glfwSetKeyCallback(window,         cb_key);
    glfwSetMouseButtonCallback(window, cb_mouse_button);
    glfwSetCursorPosCallback(window,   cb_cursor);
    glfwSetScrollCallback(window,      cb_scroll);

    // Init GLAD
    if (!gladLoadGL(glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD\n";
        return -1;
    }

    std::cout << "OpenGL " << glGetString(GL_VERSION) << "  |  "
              << glGetString(GL_RENDERER) << "\n";
    std::cout << "Controls: left-drag to orbit, scroll to zoom, ESC to quit\n";

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);

    // ── GPU resources ────────────────────────────────────────────────────────
    unsigned int VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(CUBE_VERTS), CUBE_VERTS, GL_STATIC_DRAW);

    // position  (location = 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
                          6 * sizeof(float), reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(0);
    // normal    (location = 1)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,
                          6 * sizeof(float), reinterpret_cast<void*>(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    Shader shader("shaders/basic.vert", "shaders/basic.frag");

    const glm::vec3 lightPos(3.0f, 5.0f, 3.0f);
    const glm::vec3 cubeColor(0.40f, 0.72f, 1.0f); // light blue voxel

    // ── Render loop ──────────────────────────────────────────────────────────
    while (!glfwWindowShouldClose(window)) {
        glClearColor(0.10f, 0.10f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        const float aspect = static_cast<float>(winW) / static_cast<float>(winH);
        const glm::mat4 model = glm::mat4(1.0f);
        const glm::mat4 view  = camera.getViewMatrix();
        const glm::mat4 proj  = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);

        shader.use();
        shader.setMat4("model",       model);
        shader.setMat4("view",        view);
        shader.setMat4("projection",  proj);
        shader.setVec3("lightPos",    lightPos);
        shader.setVec3("viewPos",     camera.getPosition());
        shader.setVec3("objectColor", cubeColor);

        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // ── Cleanup ──────────────────────────────────────────────────────────────
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glfwTerminate();
    return 0;
}
