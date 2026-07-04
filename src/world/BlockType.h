#pragma once

#include <cstdint>

// ── BlockType ─────────────────────────────────────────────────────────────────
// Stored as uint8_t inside Chunk — keep total count under 256.
enum class BlockType : uint8_t {
    Air = 0,

    // ── Grass variants ───────────────────────────────────────────────────────
    GrassPlain,   // standard temperate
    GrassDry,     // arid / yellowed
    GrassSnowy,   // frost-covered
    GrassDense,   // jungle / deep green

    // ── Dirt ────────────────────────────────────────────────────────────────
    Dirt,
    Sand,         // beach / desert subsurface

    // ── Rock variants ───────────────────────────────────────────────────────
    RockSmooth,   // plain stone
    RockCracked,  // weathered, darker
    RockMoss,     // mossy stone

    // ── Snow ────────────────────────────────────────────────────────────────
    Snow,

    // ── Liquids ─────────────────────────────────────────────────────────────
    Water,
    Lava,

    COUNT  // sentinel — always last
};

// ── BlockFace ─────────────────────────────────────────────────────────────────
enum class BlockFace : uint8_t {
    Top = 0, Bottom, North, South, West, East,
    COUNT
};
