#pragma once

#include "Chunk.h"
#include "../Types.h"

#include <FastNoiseLite.h>
#include <algorithm>
#include <cmath>

// ── TerrainGenerator ─────────────────────────────────────────────────────────
// Pipeline:
//   1. Temperature + humidity noise → biome selection per column
//   2. Domain-warped heightmap (biome base + amplitude + detail)
//   3. Beach override near sea level
//   4. Underground layer variety based on depth + 3D mossy noise
//   5. 3D cave carving (carve air where cave noise > threshold)
//   6. Sea-level water fill
class TerrainGenerator {
public:
    static constexpr int SEA_LEVEL = 16;

    // Public height query — used for player spawn calculation
    static int surfaceHeight(int worldX, int worldZ) {
        const auto& n = ns();
        return computeHeight(n, worldX, worldZ);
    }

    static void fill(Chunk& chunk, ChunkPos pos) {
        const auto& n = ns();

        for (int x = 0; x < CHUNK_W; ++x)
        for (int z = 0; z < CHUNK_D; ++z)
        {
            const int   wx = pos.x * CHUNK_W + x;
            const int   wz = pos.z * CHUNK_D + z;
            const int   h  = computeHeight(n, wx, wz);
            const Biome bm = biomeAt(n, wx, wz);

            // ── Solid column ─────────────────────────────────────────────────
            for (int y = 0; y <= h; ++y) {
                BlockType bt = BlockType::Air;

                if (y == h) {
                    bt = surfaceBlock(bm, h);

                } else if (y >= h - 3) {
                    // Sub-surface layer
                    bt = (bm == Biome::Desert || h <= SEA_LEVEL + 1)
                       ? BlockType::Sand
                       : BlockType::Dirt;

                } else {
                    // Deep rock — vary by depth and 3D noise
                    const float m = n.mossy.GetNoise((float)wx, (float)y * 1.2f, (float)wz);
                    if (y < h - 12) {
                        // Deep: cracked or mossy stone
                        bt = (m > 0.30f) ? BlockType::RockMoss
                           : (m < -0.35f) ? BlockType::RockCracked
                           : BlockType::RockSmooth;
                    } else {
                        bt = BlockType::RockSmooth;
                    }
                }

                // ── Cave carving ─────────────────────────────────────────
                // Only carve below the surface, leave a 2-block shell
                if (bt != BlockType::Air && y > 2 && y < h - 1) {
                    const float cv = n.cave.GetNoise((float)wx, (float)y, (float)wz);
                    if (cv > 0.52f) bt = BlockType::Air;
                }

                chunk.set(x, y, z, bt);
            }

            // ── Water fill below sea level ────────────────────────────────
            for (int y = h + 1; y <= SEA_LEVEL; ++y)
                chunk.set(x, y, z, BlockType::Water);
        }
    }

private:
    // ── Biome enumeration ─────────────────────────────────────────────────────
    enum class Biome {
        Plains,    // temperate, medium height
        Hills,     // temperate, tall and varied
        Desert,    // hot + dry, flat
        Tropical,  // hot + humid, lush
        Taiga,     // cold + humid, forested
        Tundra,    // cold + dry, flat
        Swamp,     // wet + low, near water
        Snowy,     // very cold, moderate height
    };

    // ── Biome shape parameters ────────────────────────────────────────────────
    struct BiomeParams {
        float     baseH;    // base height (blocks)
        float     ampH;     // large-feature amplitude
        float     detailAmp;// fine-detail amplitude
        BlockType surface;
        BlockType sub;      // subsurface (replaces Dirt where relevant)
    };

    static BiomeParams params(Biome b) {
        switch (b) {
            case Biome::Plains:   return { 19.0f, 5.0f, 2.0f, BlockType::GrassPlain,  BlockType::Dirt        };
            case Biome::Hills:    return { 22.0f, 10.0f,3.5f, BlockType::GrassPlain,  BlockType::Dirt        };
            case Biome::Desert:   return { 14.0f, 3.0f, 1.0f, BlockType::GrassDry,    BlockType::Sand        };
            case Biome::Tropical: return { 20.0f, 4.5f, 2.5f, BlockType::GrassDense,  BlockType::Dirt        };
            case Biome::Taiga:    return { 21.0f, 7.0f, 3.0f, BlockType::GrassDense,  BlockType::Dirt        };
            case Biome::Tundra:   return { 18.0f, 4.0f, 1.5f, BlockType::GrassSnowy,  BlockType::Dirt        };
            case Biome::Swamp:    return { 14.0f, 2.0f, 1.0f, BlockType::GrassDry,    BlockType::Dirt        };
            case Biome::Snowy:    return { 24.0f, 8.0f, 2.5f, BlockType::Snow,        BlockType::Dirt        };
            default:              return { 18.0f, 6.0f, 2.0f, BlockType::GrassPlain,  BlockType::Dirt        };
        }
    }

    static BlockType surfaceBlock(Biome b, int h) {
        // Beach override: sand near sea level regardless of biome
        if (h >= SEA_LEVEL - 1 && h <= SEA_LEVEL + 2)
            return BlockType::Sand;

        return params(b).surface;
    }

    // ── Noise collection ──────────────────────────────────────────────────────
    struct Noises {
        FastNoiseLite hills;       // large-scale terrain shape
        FastNoiseLite detail;      // fine surface detail
        FastNoiseLite temperature; // biome temperature axis
        FastNoiseLite humidity;    // biome humidity axis
        FastNoiseLite warpX;       // domain warp for X
        FastNoiseLite warpZ;       // domain warp for Z
        FastNoiseLite cave;        // 3D cave carving
        FastNoiseLite mossy;       // underground rock variety
    };

    static const Noises& ns() {
        static const Noises n = [] {
            Noises n;

            n.hills.SetSeed(1337);
            n.hills.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
            n.hills.SetFrequency(0.028f);

            n.detail.SetSeed(2718);
            n.detail.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
            n.detail.SetFrequency(0.095f);

            n.temperature.SetSeed(9876);
            n.temperature.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
            n.temperature.SetFrequency(0.011f);  // very large scale biomes

            n.humidity.SetSeed(5432);
            n.humidity.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
            n.humidity.SetFrequency(0.014f);

            n.warpX.SetSeed(1111);
            n.warpX.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
            n.warpX.SetFrequency(0.018f);

            n.warpZ.SetSeed(2222);
            n.warpZ.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
            n.warpZ.SetFrequency(0.018f);

            n.cave.SetSeed(7777);
            n.cave.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
            n.cave.SetFrequency(0.040f);

            n.mossy.SetSeed(3333);
            n.mossy.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
            n.mossy.SetFrequency(0.055f);

            return n;
        }();
        return n;
    }

    // ── Biome selection ───────────────────────────────────────────────────────
    static Biome biomeAt(const Noises& n, int wx, int wz) {
        const float t = n.temperature.GetNoise((float)wx, (float)wz); // -1..1
        const float h = n.humidity.GetNoise((float)wx, (float)wz);    // -1..1

        if (t < -0.40f) {
            return (h >  0.10f) ? Biome::Snowy : Biome::Tundra;
        }
        if (t > 0.40f) {
            return (h > 0.15f) ? Biome::Tropical : Biome::Desert;
        }
        // Temperate range
        if (h < -0.30f) return Biome::Plains;
        if (h >  0.35f) return Biome::Taiga;
        if (h > 0.10f && t > 0.05f) return Biome::Hills;
        if (h < -0.05f && t < -0.05f) return Biome::Swamp;
        return Biome::Plains;
    }

    // ── Height computation ────────────────────────────────────────────────────
    // Domain warp then sample hills + detail noise, scaled by biome params.
    // Biome blending: sample 5 points in a cross pattern and average.
    static int computeHeight(const Noises& n, int wx, int wz) {
        const float fx = (float)wx;
        const float fz = (float)wz;

        // -- Biome blending radius (in blocks) --------------------------------
        constexpr int   BLEND_RADIUS = 24;
        constexpr float INV_BLEND    = 1.0f / (5.0f); // 5 samples

        float totalH = 0.0f;
        // Sample center + 4 cardinal offsets for smooth biome transitions
        const int  offsets[5][2] = {{0,0},{BLEND_RADIUS,0},{-BLEND_RADIUS,0},{0,BLEND_RADIUS},{0,-BLEND_RADIUS}};
        const float weights[5]   = {2.0f, 0.75f, 0.75f, 0.75f, 0.75f};
        float wSum = 0.0f;

        for (int i = 0; i < 5; ++i) {
            const int   sx = wx + offsets[i][0];
            const int   sz = wz + offsets[i][1];
            const float sfx = (float)sx;
            const float sfz = (float)sz;

            // Domain warp
            const float warpAmt = 28.0f;
            const float dx = n.warpX.GetNoise(sfx, sfz)               * warpAmt;
            const float dz = n.warpZ.GetNoise(sfx + 400.0f, sfz + 400.0f) * warpAmt;

            const Biome bm  = biomeAt(n, sx, sz);
            const auto  bp  = params(bm);
            const float h = bp.baseH
                          + n.hills.GetNoise(sfx + dx, sfz + dz)  * bp.ampH
                          + n.detail.GetNoise(sfx + dx, sfz + dz) * bp.detailAmp;

            totalH += h * weights[i];
            wSum   += weights[i];
        }

        const float blended = totalH / wSum;
        return std::clamp(static_cast<int>(std::round(blended)), 2, CHUNK_H - 2);
    }
};
