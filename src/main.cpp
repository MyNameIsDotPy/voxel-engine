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
bool         g_noclip   = false;  // toggled by F key

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

    // Movement keys
    const bool held = (action != GLFW_RELEASE);
    if (key == GLFW_KEY_W)             g_input.forward  = held;
    if (key == GLFW_KEY_S)             g_input.backward = held;
    if (key == GLFW_KEY_A)             g_input.left     = held;
    if (key == GLFW_KEY_D)             g_input.right    = held;
    if (key == GLFW_KEY_SPACE)         g_input.jump     = held;
    if (key == GLFW_KEY_LEFT_CONTROL)  g_input.down     = held;
    if (key == GLFW_KEY_LEFT_SHIFT)    g_input.sprint   = held;

    // Noclip toggle
    if (key == GLFW_KEY_F && action == GLFW_PRESS) {
        // player is not accessible here — toggle is read in the render loop via a global flag
        extern bool g_noclip;
        g_noclip = !g_noclip;
        std::cout << "[Noclip] " << (g_noclip ? "ON" : "OFF") << "\n";
    }
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
struct ChunkRenderData { ChunkMesh opaque; ChunkMesh transparent; };
static std::unordered_map<ChunkPos, ChunkRenderData, ChunkPosHash> g_meshes;

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

        auto data = MeshBuilder::build(chunk, pos, neighbors);

        auto& rd = g_meshes[pos];
        rd.opaque.upload(data.opaque);
        rd.transparent.upload(data.transparent);
        chunk.clearDirty();
    }

    // Remove meshes for unloaded chunks
    for (auto it = g_meshes.begin(); it != g_meshes.end(); )
        it = world.getChunk(it->first) ? ++it : g_meshes.erase(it);
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
    std::cout << "WASD = move  |  Space = jump/swim up  |  LCtrl = fly down  |  Shift = sprint\n";
    std::cout << "F = noclip toggle  |  V = debug mode  |  ESC = quit\n";

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
    Shader overlayShader("shaders/overlay.vert", "shaders/overlay.frag");

    // ── Full-screen overlay quad (NDC) ────────────────────────────────────────
    // Used for the underwater tint
    {
        // will be set up below
    }
    unsigned int overlayVAO, overlayVBO;
    {
        const float quad[] = {
            -1.f,-1.f,   1.f,-1.f,   1.f, 1.f,
            -1.f,-1.f,   1.f, 1.f,  -1.f, 1.f,
        };
        glGenVertexArrays(1, &overlayVAO);
        glGenBuffers(1, &overlayVBO);
        glBindVertexArray(overlayVAO);
        glBindBuffer(GL_ARRAY_BUFFER, overlayVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
        glEnableVertexAttribArray(0);
        glBindVertexArray(0);
    }

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
        player.noclip = g_noclip;
        player.handleInput(g_input, lookForward(), lookRight(), world);
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
        const bool underwater = player.isInWater(world);

        glClearColor(0.45f, 0.70f, 0.95f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader.use();
        shader.setMat4("model",      model);
        shader.setMat4("view",       view);
        shader.setMat4("projection", proj);
        shader.setVec3("lightPos",   lightPos);
        shader.setVec3("viewPos",    eye);

        // ── Pass 1: opaque geometry ───────────────────────────────────────────
        if (debugMode == DebugMode::Solid) {
            for (auto& [pos, rd] : g_meshes) rd.opaque.draw();
        } else {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            for (auto& [pos, rd] : g_meshes) rd.opaque.draw();

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
            for (auto& [pos, rd] : g_meshes) rd.opaque.draw();
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            glLineWidth(1.0f); glPointSize(1.0f);
        }

        // ── Pass 2: transparent geometry (water) ──────────────────────────────
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);

        shader.use();
        for (auto& [pos, rd] : g_meshes) rd.transparent.draw();

        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);

        // ── Pass 3: underwater screen tint ────────────────────────────────────
        if (underwater) {
            // Project the water surface plane onto the screen to find where
            // the tint should stop (above the water line = no tint).
            const float waterWorldY = static_cast<float>(TerrainGenerator::SEA_LEVEL) + 1.0f;

            float waterLineNDC = 2.0f; // default: full screen tinted

            glm::vec3 fwdFlat(lookForward().x, 0.0f, lookForward().z);
            const float fwdLen = glm::length(fwdFlat);
            if (fwdLen > 0.01f) {
                // Place a point on the water surface 50 units in front (horizontally)
                fwdFlat = glm::normalize(fwdFlat) * 50.0f;
                const glm::vec3 wp(eye.x + fwdFlat.x, waterWorldY, eye.z + fwdFlat.z);
                const glm::vec4 clip = proj * view * glm::vec4(wp, 1.0f);
                if (clip.w > 0.001f)
                    waterLineNDC = glm::clamp(clip.y / clip.w, -1.5f, 3.0f);
            }

            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glDisable(GL_DEPTH_TEST);

            overlayShader.use();
            overlayShader.setVec4("overlayColor", glm::vec4(0.04f, 0.22f, 0.55f, 0.45f));
            overlayShader.setFloat("waterLineNDC", waterLineNDC);
            overlayShader.setFloat("screenHeight", static_cast<float>(winH));

            glBindVertexArray(overlayVAO);
            glDrawArrays(GL_TRIANGLES, 0, 6);

            glEnable(GL_DEPTH_TEST);
            glDisable(GL_BLEND);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
