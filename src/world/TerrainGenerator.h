#pragma once

#include "Chunk.h"
#include "../Types.h"

#include <FastNoiseLite.h>
#include <algorithm>
#include <cmath>

class TerrainGenerator {
public:
    static int surfaceHeight(int worldX, int worldZ) {
        static FastNoiseLite hills = [] {
            FastNoiseLite n;
            n.SetSeed(1337);
            n.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
            n.SetFrequency(0.035f);
            return n;
        }();

        static FastNoiseLite detail = [] {
            FastNoiseLite n;
            n.SetSeed(2718);
            n.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
            n.SetFrequency(0.11f);
            return n;
        }();

        const float height = 18.0f + hills.GetNoise((float)worldX, (float)worldZ) * 10.0f
                                   + detail.GetNoise((float)worldX, (float)worldZ) * 3.0f;
        return std::clamp((int)std::round(height), 2, CHUNK_H - 2);
    }

    static void fill(Chunk& chunk, ChunkPos pos) {
        for (int x = 0; x < CHUNK_W; ++x) {
            for (int z = 0; z < CHUNK_D; ++z) {
                const int worldX = pos.x * CHUNK_W + x;
                const int worldZ = pos.z * CHUNK_D + z;
                const int h = surfaceHeight(worldX, worldZ);

                for (int y = 0; y <= h; ++y) {
                    if (y == h) {
                        chunk.set(x, y, z, surfaceBlock(h, worldX, worldZ));
                    } else if (y > h - 4) {
                        chunk.set(x, y, z, BlockType::Dirt);
                    } else {
                        chunk.set(x, y, z, BlockType::RockSmooth);
                    }
                }
            }
        }
    }

private:
    static BlockType surfaceBlock(int height, int worldX, int worldZ) {
        static FastNoiseLite biome = [] {
            FastNoiseLite n;
            n.SetSeed(4242);
            n.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
            n.SetFrequency(0.055f);
            return n;
        }();

        if (height > 24) {
            return BlockType::Snow;
        }
        if (height < 14) {
            return BlockType::GrassDry;
        }
        if (biome.GetNoise((float)worldX, (float)worldZ) > 0.35f) {
            return BlockType::GrassDense;
        }
        return BlockType::GrassPlain;
    }
};
