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

    static void fill(Chunk& chunk, ChunkPos /*pos*/) {
        // Flat world: single layer of grass at y = 0
        for (int x = 0; x < CHUNK_W; ++x)
        for (int z = 0; z < CHUNK_D; ++z)
            chunk.set(x, 0, z, BlockType::Grass);
    }
};
