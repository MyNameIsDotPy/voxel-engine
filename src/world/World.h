#pragma once

#include "Chunk.h"
#include "../Types.h"

#include <unordered_map>
#include <glm/glm.hpp>

// ── World ─────────────────────────────────────────────────────────────────────
// Manages the chunk map and provides cross-chunk block access.
// Full implementation tracked in issue #11.
class World {
public:
    static constexpr int RENDER_DISTANCE = 8; // chunks

    // Update chunk load/unload based on player position
    void update(glm::vec3 playerPos);

    // Cross-chunk safe block accessors
    BlockType getBlock(glm::ivec3 worldPos) const;
    void      setBlock(glm::ivec3 worldPos, BlockType type);

    // Raw chunk access (returns nullptr if not loaded)
    Chunk*       getChunk(ChunkPos pos);
    const Chunk* getChunk(ChunkPos pos) const;

    // Iteration — used by Renderer to walk all loaded chunks
    const std::unordered_map<ChunkPos, Chunk, ChunkPosHash>& chunks() const {
        return m_chunks;
    }

    // Convert world block position to ChunkPos + local coords
    static ChunkPos toChunkPos(glm::ivec3 worldPos) {
        return { (int)std::floor(worldPos.x / (float)CHUNK_W),
                 (int)std::floor(worldPos.z / (float)CHUNK_D) };
    }

    static glm::ivec3 toLocalPos(glm::ivec3 worldPos) {
        int lx = ((worldPos.x % CHUNK_W) + CHUNK_W) % CHUNK_W;
        int lz = ((worldPos.z % CHUNK_D) + CHUNK_D) % CHUNK_D;
        return { lx, worldPos.y, lz };
    }

private:
    std::unordered_map<ChunkPos, Chunk, ChunkPosHash> m_chunks;
};
