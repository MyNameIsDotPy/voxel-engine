#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>
#include <unordered_map>

#include "Shader.h"
#include "Camera.h"
#include "Types.h"
#include "world/World.h"
#include "render/ChunkMesh.h"
#include "render/MeshBuilder.h"

// ── Window config ─────────────────────────────────────────────────────────────
constexpr int  WIN_W  = 1280;
constexpr int  WIN_H  = 720;
constexpr char WIN_T[] = "Voxel Engine";

// ── Debug draw mode ───────────────────────────────────────────────────────────
enum class DebugMode { Solid = 0, Wireframe, Vertices, COUNT };
static DebugMode debugMode = DebugMode::Solid;
static const char* debugModeLabel[] = { "Solid", "Wireframe", "Vertices" };

static void applyDebugMode() {
    std::cout << "[Debug] Mode: " << debugModeLabel[static_cast<int>(debugMode)] << "\n";
}

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

    if (key == GLFW_KEY_V && action == GLFW_PRESS) {
        int next = (static_cast<int>(debugMode) + 1) % static_cast<int>(DebugMode::COUNT);
        debugMode = static_cast<DebugMode>(next);
        applyDebugMode();
    }
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

// ── Chunk mesh cache ──────────────────────────────────────────────────────────
static std::unordered_map<ChunkPos, ChunkMesh, ChunkPosHash> g_meshes;

static void rebuildDirtyMeshes(World& world) {
    for (auto& [pos, chunk] : world.chunks()) {
        if (!chunk.isDirty()) continue;

        // Wire up horizontal neighbors so shared boundary faces are culled.
        // Order must match MeshBuilder::FACES: +Y,-Y,+X,-X,+Z,-Z
        const Chunk* neighbors[6] = {
            nullptr,                                     // +Y — no vertical stacking yet
            nullptr,                                     // -Y
            world.getChunk({pos.x + 1, pos.z}),         // +X East
            world.getChunk({pos.x - 1, pos.z}),         // -X West
            world.getChunk({pos.x,     pos.z + 1}),     // +Z South
            world.getChunk({pos.x,     pos.z - 1}),     // -Z North
        };

        auto verts = MeshBuilder::build(chunk, pos, neighbors);

        auto it = g_meshes.find(pos);
        if (it == g_meshes.end())
            it = g_meshes.emplace(std::piecewise_construct,
                                  std::forward_as_tuple(pos),
                                  std::forward_as_tuple()).first;

        it->second.upload(verts);
        chunk.clearDirty();

        std::cout << "[Mesh] Chunk (" << pos.x << "," << pos.z
                  << ") rebuilt — " << verts.size() << " vertices\n";
    }
}

// ── main ─────────────────────────────────────────────────────────────────────
int main() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);

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

    if (!gladLoadGL(glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD\n";
        return -1;
    }

    std::cout << "OpenGL " << glGetString(GL_VERSION) << "  |  "
              << glGetString(GL_RENDERER) << "\n";
    std::cout << "Controls: left-drag to orbit, scroll to zoom, ESC to quit\n";
    std::cout << "Debug   : V → cycle Solid / Wireframe / Vertices\n";

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_CULL_FACE);   // skip back faces — MeshBuilder emits CCW quads

    // ── World setup — single 16x16 chunk ─────────────────────────────────────
    World world;
    ChunkPos origin{0, 0};
    Chunk& c = world.chunks()[origin];
    TerrainGenerator::fill(c, origin);

    rebuildDirtyMeshes(world);

    // ── Camera — look down at the flat chunk ──────────────────────────────────
    camera.target   = glm::vec3(CHUNK_W * 0.5f, 0.0f, CHUNK_D * 0.5f);
    camera.distance = 24.0f;
    camera.pitch    = 45.0f;
    camera.yaw      = 45.0f;

    // ── Shaders ───────────────────────────────────────────────────────────────
    Shader shader("shaders/basic.vert", "shaders/basic.frag");
    Shader debugShader("shaders/debug.vert", "shaders/debug.frag");

    const glm::vec3 lightPos (8.0f, 20.0f,  8.0f);
    const glm::vec3 wireColor (1.0f,  1.0f,  0.0f);
    const glm::vec3 pointColor(1.0f,  0.3f,  0.3f);

    // ── Render loop ───────────────────────────────────────────────────────────
    while (!glfwWindowShouldClose(window)) {
        glClearColor(0.45f, 0.70f, 0.95f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        const float     aspect = static_cast<float>(winW) / static_cast<float>(winH);
        const glm::mat4 model  = glm::mat4(1.0f);
        const glm::mat4 view   = camera.getViewMatrix();
        const glm::mat4 proj   = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 200.0f);

        if (debugMode == DebugMode::Solid) {
            shader.use();
            shader.setMat4("model",      model);
            shader.setMat4("view",       view);
            shader.setMat4("projection", proj);
            shader.setVec3("lightPos",   lightPos);
            shader.setVec3("viewPos",    camera.getPosition());
            for (auto& [pos, mesh] : g_meshes) mesh.draw();
        } else {
            // Dim solid base pass
            shader.use();
            shader.setMat4("model",      model);
            shader.setMat4("view",       view);
            shader.setMat4("projection", proj);
            shader.setVec3("lightPos",   lightPos);
            shader.setVec3("viewPos",    camera.getPosition());
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            for (auto& [pos, mesh] : g_meshes) mesh.draw();

            // Debug overlay pass
            debugShader.use();
            debugShader.setMat4("model",      model);
            debugShader.setMat4("view",       view);
            debugShader.setMat4("projection", proj);

            if (debugMode == DebugMode::Wireframe) {
                debugShader.setVec3("debugColor", wireColor);
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                glLineWidth(1.5f);
            } else {
                debugShader.setVec3("debugColor", pointColor);
                glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
                glPointSize(5.0f);
            }
            for (auto& [pos, mesh] : g_meshes) mesh.draw();

            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            glLineWidth(1.0f);
            glPointSize(1.0f);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
