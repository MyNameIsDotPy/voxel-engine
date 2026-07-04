#pragma once

#include <cstdint>

// ── Block types ───────────────────────────────────────────────────────────────
// Stored as uint8_t inside Chunk — keep total count under 256.
enum class BlockType : uint8_t {
    Air = 0,
    Grass,
    Dirt,
    Stone,
    Sand,
    Gravel,
    Water,
    Wood,
    Leaves,
    COUNT   // sentinel — always last
};

// ── Face indices used for per-face texture lookup ────────────────────────────
enum class BlockFace : uint8_t {
    Top = 0,
    Bottom,
    North,  // -Z
    South,  // +Z
    West,   // -X
    East,   // +X
    COUNT
};

// ── Per-block static properties ───────────────────────────────────────────────
struct BlockDef {
    const char* name;
    bool        solid;       // blocks movement and face culling
    bool        transparent; // faces next to this block are still rendered
    // Texture atlas row index per face (matched to TextureAtlas layout)
    uint8_t     textures[static_cast<int>(BlockFace::COUNT)];
};
