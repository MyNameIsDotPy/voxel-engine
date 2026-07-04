#pragma once

#include "Chunk.h"
#include "../Types.h"

// ── TerrainGenerator ─────────────────────────────────────────────────────────
// Fills a Chunk with procedural terrain.
// Full noise-based generation is tracked in issue #12.
// This version produces a flat test terrain so the pipeline can be verified.
class TerrainGenerator {
public:
    // surface height (in blocks) at chunk-local column (x, z)
    // Override with noise in issue #12.
    static int surfaceHeight(int /*worldX*/, int /*worldZ*/) {
        return 6; // flat terrain at y = 6
    }

    static void fill(Chunk& chunk, ChunkPos pos) {
        for (int x = 0; x < CHUNK_W; ++x)
        for (int z = 0; z < CHUNK_D; ++z)
        {
            const int worldX = pos.x * CHUNK_W + x;
            const int worldZ = pos.z * CHUNK_D + z;
            const int surface = surfaceHeight(worldX, worldZ);

            for (int y = 0; y < CHUNK_H; ++y) {
                BlockType bt = BlockType::Air;

                if      (y == surface)     bt = BlockType::Grass;
                else if (y >= surface - 3) bt = BlockType::Dirt;
                else if (y > 0)            bt = BlockType::Stone;

                chunk.set(x, y, z, bt);
            }
        }
    }
};
