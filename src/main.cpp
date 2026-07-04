#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>
#include <unordered_map>

#include "Shader.h"
#include "Types.h"
#include "world/World.h"
#include "world/Player.h"
#include "world/TerrainGenerator.h"
#include "world/RayCast.h"
#include "render/ChunkMesh.h"
#include "render/MeshBuilder.h"

// ── Window config ─────────────────────────────────────────────────────────────
constexpr int  WIN_W  = 1280;
constexpr int  WIN_H  = 720;
constexpr char WIN_T[] = "Voxel Engine";

// ── Debug draw mode ───────────────────────────────────────────────────────────
enum class DebugMode { Solid = 0, Wireframe, Vertices, COUNT };
static DebugMode    debugMode = DebugMode::Solid;
static const char*  debugModeLabel[] = { "Solid", "Wireframe", "Vertices" };

// ── First-person look ─────────────────────────────────────────────────────────
static float yaw   = -90.0f; // degrees, starts facing -Z
static float pitch =   0.0f; // degrees, clamped ±89

static glm::vec3 lookForward() {
    const float y = glm::radians(yaw);
    const float p = glm::radians(pitch);
    return glm::normalize(glm::vec3(
        std::cos(p) * std::cos(y),
        std::sin(p),
        std::cos(p) * std::sin(y)
    ));
}

static glm::vec3 lookRight() {
    return glm::normalize(glm::cross(lookForward(), glm::vec3(0,1,0)));
}

// ── Input state ───────────────────────────────────────────────────────────────
static PlayerInput g_input;

// ── Window state ─────────────────────────────────────────────────────────────
static int   winW = WIN_W;
static int   winH = WIN_H;
static float lastX = WIN_W / 2.0f;
static float lastY = WIN_H / 2.0f;
static bool  firstMouse = true;

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
        std::cout << "[Debug] Mode: " << debugModeLabel[static_cast<int>(debugMode)] << "\n";
    }

    // Movement keys — track held state
    const bool held = (action != GLFW_RELEASE);
    if (key == GLFW_KEY_W)          g_input.forward  = held;
    if (key == GLFW_KEY_S)          g_input.backward = held;
    if (key == GLFW_KEY_A)          g_input.left     = held;
    if (key == GLFW_KEY_D)          g_input.right    = held;
    if (key == GLFW_KEY_SPACE)      g_input.jump     = held;
    if (key == GLFW_KEY_LEFT_SHIFT) g_input.sprint   = held;
}

static void cb_cursor(GLFWwindow*, double xpos, double ypos) {
    if (firstMouse) {
        lastX = static_cast<float>(xpos);
        lastY = static_cast<float>(ypos);
        firstMouse = false;
    }

    const float dx = static_cast<float>(xpos) - lastX;
    const float dy = lastY - static_cast<float>(ypos); // inverted Y
    lastX = static_cast<float>(xpos);
    lastY = static_cast<float>(ypos);

    yaw   += dx * 0.12f;
    pitch += dy * 0.12f;
    pitch  = std::clamp(pitch, -89.0f, 89.0f);
}

// ── Chunk mesh cache ──────────────────────────────────────────────────────────
static std::unordered_map<ChunkPos, ChunkMesh, ChunkPosHash> g_meshes;

static void rebuildDirtyMeshes(World& world) {
    for (auto& [pos, chunk] : world.chunks()) {
        if (!chunk.isDirty()) continue;

        const Chunk* neighbors[6] = {
            nullptr,
            nullptr,
            world.getChunk({pos.x + 1, pos.z}),
            world.getChunk({pos.x - 1, pos.z}),
            world.getChunk({pos.x,     pos.z + 1}),
            world.getChunk({pos.x,     pos.z - 1}),
        };

        auto verts = MeshBuilder::build(chunk, pos, neighbors);

        auto it = g_meshes.find(pos);
        if (it == g_meshes.end())
            it = g_meshes.emplace(std::piecewise_construct,
                                  std::forward_as_tuple(pos),
                                  std::forward_as_tuple()).first;

        it->second.upload(verts);
        chunk.clearDirty();
    }

    // Remove meshes for unloaded chunks
    for (auto it = g_meshes.begin(); it != g_meshes.end(); ) {
        if (!world.getChunk(it->first)) it = g_meshes.erase(it);
        else ++it;
    }
}

// ── main ─────────────────────────────────────────────────────────────────────
int main() {
    if (!glfwInit()) { std::cerr << "Failed to init GLFW\n"; return -1; }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(WIN_W, WIN_H, WIN_T, nullptr, nullptr);
    if (!window) { std::cerr << "Failed to create window\n"; glfwTerminate(); return -1; }

    glfwMakeContextCurrent(window);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); // capture cursor
    glfwSetFramebufferSizeCallback(window, cb_framebuffer);
    glfwSetKeyCallback(window,         cb_key);
    glfwSetCursorPosCallback(window,   cb_cursor);

    if (!gladLoadGL(glfwGetProcAddress)) { std::cerr << "Failed to init GLAD\n"; return -1; }

    std::cout << "OpenGL " << glGetString(GL_VERSION) << "  |  " << glGetString(GL_RENDERER) << "\n";
    std::cout << "WASD = move  |  Space = jump  |  Shift = sprint  |  V = debug  |  ESC = quit\n";

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_CULL_FACE);

    // ── World + Player ────────────────────────────────────────────────────────
    World  world;
    Player player;

    // Spawn above the highest terrain block in the player's footprint
    {
        int maxH = 0;
        for (int x = (int)player.position.x - 1; x <= (int)player.position.x + 1; ++x)
        for (int z = (int)player.position.z - 1; z <= (int)player.position.z + 1; ++z)
            maxH = std::max(maxH, TerrainGenerator::surfaceHeight(x, z));
        player.position.y = static_cast<float>(maxH) + player.halfExtents.y + 1.5f;
    }

    world.update(player.position);
    rebuildDirtyMeshes(world);

    // ── Shaders ───────────────────────────────────────────────────────────────
    Shader shader("shaders/basic.vert", "shaders/basic.frag");
    Shader debugShader("shaders/debug.vert", "shaders/debug.frag");

    const glm::vec3 lightPos  ( 64.0f, 80.0f,  64.0f);
    const glm::vec3 wireColor ( 1.0f,  1.0f,   0.0f);
    const glm::vec3 pointColor( 1.0f,  0.3f,   0.3f);

    double lastTime = glfwGetTime();

    // ── Render loop ───────────────────────────────────────────────────────────
    while (!glfwWindowShouldClose(window)) {
        const double now = glfwGetTime();
        const float  dt  = static_cast<float>(now - lastTime);
        lastTime = now;

        // ── Update ───────────────────────────────────────────────────────────
        player.handleInput(g_input, lookForward(), lookRight());
        player.update(dt, world);
        world.update(player.position);
        rebuildDirtyMeshes(world);

        // ── View / projection ─────────────────────────────────────────────────
        const glm::vec3 eye    = player.eyePos();
        const glm::mat4 view   = glm::lookAt(eye, eye + lookForward(), glm::vec3(0,1,0));
        const float     aspect = static_cast<float>(winW) / static_cast<float>(winH);
        const glm::mat4 proj   = glm::perspective(glm::radians(90.0f), aspect, 0.05f, 500.0f);
        const glm::mat4 model  = glm::mat4(1.0f);

        // ── Draw ──────────────────────────────────────────────────────────────
        glClearColor(0.45f, 0.70f, 0.95f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader.use();
        shader.setMat4("model",      model);
        shader.setMat4("view",       view);
        shader.setMat4("projection", proj);
        shader.setVec3("lightPos",   lightPos);
        shader.setVec3("viewPos",    eye);

        if (debugMode == DebugMode::Solid) {
            for (auto& [pos, mesh] : g_meshes) mesh.draw();
        } else {
            // Dim base
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            for (auto& [pos, mesh] : g_meshes) mesh.draw();

            // Overlay
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
