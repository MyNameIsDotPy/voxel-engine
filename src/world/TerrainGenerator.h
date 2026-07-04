#pragma once

#include "Chunk.h"
#include "../Types.h"

// ── TerrainGenerator ─────────────────────────────────────────────────────────
// Fills a Chunk with test terrain showing every voxel type.
// Full noise-based generation is tracked in issue #12.
class TerrainGenerator {
public:
    static void fill(Chunk& chunk, ChunkPos /*pos*/) {
        // Divide the 16x16 chunk into a 4x3 grid of zones (4 cols, 3 rows)
        // to show every block type side by side.
        //
        //  z\x  0..3    4..7    8..11   12..15
        //  0..4  GrassP  GrassDr GrassSn GrassDn
        //  5..9  Dirt    RockSm  RockCr  RockMs
        // 10..15 Snow    Water   Lava    (Snow)

        for (int x = 0; x < CHUNK_W; ++x)
        for (int z = 0; z < CHUNK_D; ++z)
        {
            const int col = x / 4;  // 0..3
            const int row = z / 5;  // 0..2

            BlockType bt = BlockType::GrassPlain;

            if (row == 0) {
                const BlockType cols[] = {
                    BlockType::GrassPlain,
                    BlockType::GrassDry,
                    BlockType::GrassSnowy,
                    BlockType::GrassDense,
                };
                bt = cols[col];
            } else if (row == 1) {
                const BlockType cols[] = {
                    BlockType::Dirt,
                    BlockType::RockSmooth,
                    BlockType::RockCracked,
                    BlockType::RockMoss,
                };
                bt = cols[col];
            } else {
                const BlockType cols[] = {
                    BlockType::Snow,
                    BlockType::Water,
                    BlockType::Lava,
                    BlockType::Snow,
                };
                bt = cols[col];
            }

            chunk.set(x, 0, z, bt);
        }
    }
};
