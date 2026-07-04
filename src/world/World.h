#pragma once

#include "Chunk.h"
#include "TerrainGenerator.h"
#include "BlockRegistry.h"
#include "../Types.h"

#include <unordered_map>
#include <glm/glm.hpp>
#include <cmath>

// ── World ─────────────────────────────────────────────────────────────────────
// Manages the chunk map and provides cross-chunk block access.
class World {
public:
    static constexpr int RENDER_DISTANCE = 8; // chunks

    // ── Chunk lifecycle ───────────────────────────────────────────────────────

    // Load/unload chunks based on player position. Call once per frame.
    void update(glm::vec3 playerPos) {
        const ChunkPos center = worldToChunkPos(playerPos);

        // Load missing chunks within render distance
        for (int dx = -RENDER_DISTANCE; dx <= RENDER_DISTANCE; ++dx)
        for (int dz = -RENDER_DISTANCE; dz <= RENDER_DISTANCE; ++dz)
        {
            if (dx * dx + dz * dz > RENDER_DISTANCE * RENDER_DISTANCE) continue;

            ChunkPos cp{ center.x + dx, center.z + dz };
            if (m_chunks.find(cp) == m_chunks.end()) {
                Chunk& c = m_chunks[cp];
                TerrainGenerator::fill(c, cp);

                // New chunk arrived — force neighbors to re-mesh so their
                // boundary water/solid faces update correctly
                constexpr int off[4][2] = {{1,0},{-1,0},{0,1},{0,-1}};
                for (auto& o : off) {
                    auto it = m_chunks.find({cp.x + o[0], cp.z + o[1]});
                    if (it != m_chunks.end()) it->second.markDirty();
                }
            }
        }

        // Unload distant chunks
        for (auto it = m_chunks.begin(); it != m_chunks.end(); ) {
            const int dx = it->first.x - center.x;
            const int dz = it->first.z - center.z;
            if (dx * dx + dz * dz > (RENDER_DISTANCE + 2) * (RENDER_DISTANCE + 2))
                it = m_chunks.erase(it);
            else
                ++it;
        }
    }

    // ── Block accessors ───────────────────────────────────────────────────────

    BlockType getBlock(glm::ivec3 worldPos) const {
        if (worldPos.y < 0 || worldPos.y >= CHUNK_H) return BlockType::Air;
        const ChunkPos cp  = worldToChunkPos(worldPos);
        const Chunk*   c   = getChunk(cp);
        if (!c) return BlockType::Air;
        const glm::ivec3 lp = toLocal(worldPos);
        return c->get(lp.x, lp.y, lp.z);
    }

    void setBlock(glm::ivec3 worldPos, BlockType type) {
        if (worldPos.y < 0 || worldPos.y >= CHUNK_H) return;
        const ChunkPos cp = worldToChunkPos(worldPos);
        Chunk* c = getChunk(cp);
        if (!c) return;
        const glm::ivec3 lp = toLocal(worldPos);
        c->set(lp.x, lp.y, lp.z, type);

        // Mark edge neighbors dirty if the modified block sits on a chunk border
        markNeighborsDirty(cp, lp);
    }

    bool isSolid(glm::ivec3 worldPos) const {
        return BlockRegistry::get(getBlock(worldPos)).isSolid();
    }

    // ── Chunk access ──────────────────────────────────────────────────────────

    Chunk* getChunk(ChunkPos pos) {
        auto it = m_chunks.find(pos);
        return (it != m_chunks.end()) ? &it->second : nullptr;
    }

    const Chunk* getChunk(ChunkPos pos) const {
        auto it = m_chunks.find(pos);
        return (it != m_chunks.end()) ? &it->second : nullptr;
    }

    // Iteration — Renderer walks this to draw all loaded chunks
    const std::unordered_map<ChunkPos, Chunk, ChunkPosHash>& chunks() const {
        return m_chunks;
    }
    std::unordered_map<ChunkPos, Chunk, ChunkPosHash>& chunks() {
        return m_chunks;
    }

    // ── Coordinate helpers ────────────────────────────────────────────────────

    static ChunkPos worldToChunkPos(glm::vec3 worldPos) {
        return { (int)std::floor(worldPos.x / CHUNK_W),
                 (int)std::floor(worldPos.z / CHUNK_D) };
    }

    static ChunkPos worldToChunkPos(glm::ivec3 worldPos) {
        return { (int)std::floor((float)worldPos.x / CHUNK_W),
                 (int)std::floor((float)worldPos.z / CHUNK_D) };
    }

    static glm::ivec3 toLocal(glm::ivec3 worldPos) {
        int lx = ((worldPos.x % CHUNK_W) + CHUNK_W) % CHUNK_W;
        int lz = ((worldPos.z % CHUNK_D) + CHUNK_D) % CHUNK_D;
        return { lx, worldPos.y, lz };
    }

private:
    std::unordered_map<ChunkPos, Chunk, ChunkPosHash> m_chunks;

    void markNeighborsDirty(ChunkPos cp, glm::ivec3 lp) {
        auto markIf = [&](bool cond, int ddx, int ddz) {
            if (!cond) return;
            ChunkPos n{ cp.x + ddx, cp.z + ddz };
            Chunk* c = getChunk(n);
            if (c) c->markDirty();
        };
        markIf(lp.x == 0,           -1,  0);
        markIf(lp.x == CHUNK_W - 1,  1,  0);
        markIf(lp.z == 0,             0, -1);
        markIf(lp.z == CHUNK_D - 1,   0,  1);
    }
};
