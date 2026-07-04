#pragma once

#include <glm/glm.hpp>

// ── Shared types used across the rendering and world layers ──────────────────

// Single vertex sent to the GPU
struct Vertex {
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec2 uv;
};

// Integer chunk coordinate in the world grid (XZ plane)
struct ChunkPos {
    int x, z;

    bool operator==(const ChunkPos& o) const { return x == o.x && z == o.z; }
};

struct ChunkPosHash {
    std::size_t operator()(const ChunkPos& p) const noexcept {
        // Cantor pairing-inspired hash
        std::size_t hx = std::hash<int>{}(p.x);
        std::size_t hz = std::hash<int>{}(p.z);
        return hx ^ (hz * 2654435761u);
    }
};

// Axis-aligned bounding box (world space)
struct AABB {
    glm::vec3 min;
    glm::vec3 max;
};
