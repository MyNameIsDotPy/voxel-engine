#pragma once

#include "BlockType.h"
#include "../Types.h"

#include <array>
#include <cassert>

// ── Chunk constants ───────────────────────────────────────────────────────────
static constexpr int CHUNK_W = 16;  // X
static constexpr int CHUNK_H = 256; // Y
static constexpr int CHUNK_D = 16;  // Z

// ── Chunk ─────────────────────────────────────────────────────────────────────
// Stores a 16×256×16 volume of block types.
// The dirty flag signals MeshBuilder that the mesh must be rebuilt.
class Chunk {
public:
    Chunk() { m_data.fill(BlockType::Air); }

    // ── Block access ─────────────────────────────────────────────────────────
    BlockType get(int x, int y, int z) const {
        assert(inBounds(x, y, z));
        return m_data[index(x, y, z)];
    }

    void set(int x, int y, int z, BlockType type) {
        assert(inBounds(x, y, z));
        m_data[index(x, y, z)] = type;
        m_dirty = true;
    }

    // Safe getter — returns Air for out-of-bounds queries (used by MeshBuilder
    // when checking neighbors at chunk edges)
    BlockType getSafe(int x, int y, int z) const {
        if (!inBounds(x, y, z)) return BlockType::Air;
        return m_data[index(x, y, z)];
    }

    // ── Dirty flag ───────────────────────────────────────────────────────────
    bool isDirty()    const { return m_dirty; }
    void clearDirty()       { m_dirty = false; }
    void markDirty()        { m_dirty = true; }

    // ── World-space AABB of this chunk ───────────────────────────────────────
    // chunkPos is the chunk's grid coordinate (not block coordinate)
    AABB bounds(ChunkPos pos) const {
        return {
            glm::vec3(pos.x * CHUNK_W,       0,       pos.z * CHUNK_D),
            glm::vec3(pos.x * CHUNK_W + CHUNK_W, CHUNK_H, pos.z * CHUNK_D + CHUNK_D)
        };
    }

    // ── Utilities ────────────────────────────────────────────────────────────
    static bool inBounds(int x, int y, int z) {
        return x >= 0 && x < CHUNK_W &&
               y >= 0 && y < CHUNK_H &&
               z >= 0 && z < CHUNK_D;
    }

private:
    static int index(int x, int y, int z) {
        return x + CHUNK_W * (z + CHUNK_D * y);
    }

    std::array<BlockType, CHUNK_W * CHUNK_H * CHUNK_D> m_data;
    bool m_dirty = true; // new chunks always need an initial mesh build
};
