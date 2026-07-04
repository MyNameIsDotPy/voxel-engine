#pragma once

#include "BlockType.h"
#include <cassert>

// ── BlockRegistry ─────────────────────────────────────────────────────────────
// Static lookup table: BlockType → BlockDef.
// Access via BlockRegistry::get(BlockType).
class BlockRegistry {
public:
    static const BlockDef& get(BlockType type) {
        const int idx = static_cast<int>(type);
        assert(idx >= 0 && idx < static_cast<int>(BlockType::COUNT));
        return s_defs[idx];
    }

    // Texture atlas slot indices (must match the order textures are packed)
    // Slot layout:
    //   0  grass_top
    //   1  grass_side
    //   2  dirt
    //   3  stone
    //   4  sand
    //   5  gravel
    //   6  water
    //   7  wood_top
    //   8  wood_side
    //   9  leaves

private:
    // T=Top, Bo=Bottom, N=North, S=South, W=West, E=East
    // Indices correspond to atlas slots above
    inline static const BlockDef s_defs[] = {
        // Air
        { "air",    false, true,  { 0, 0, 0, 0, 0, 0 } },
        // Grass  (top=grass_top, bottom=dirt, sides=grass_side)
        { "grass",  true,  false, { 0, 2, 1, 1, 1, 1 } },
        // Dirt
        { "dirt",   true,  false, { 2, 2, 2, 2, 2, 2 } },
        // Stone
        { "stone",  true,  false, { 3, 3, 3, 3, 3, 3 } },
        // Sand
        { "sand",   true,  false, { 4, 4, 4, 4, 4, 4 } },
        // Gravel
        { "gravel", true,  false, { 5, 5, 5, 5, 5, 5 } },
        // Water
        { "water",  false, true,  { 6, 6, 6, 6, 6, 6 } },
        // Wood  (top/bottom=wood_top, sides=wood_side)
        { "wood",   true,  false, { 7, 7, 8, 8, 8, 8 } },
        // Leaves
        { "leaves", true,  true,  { 9, 9, 9, 9, 9, 9 } },
    };

    static_assert(
        sizeof(s_defs) / sizeof(s_defs[0]) == static_cast<int>(BlockType::COUNT),
        "BlockRegistry::s_defs is out of sync with BlockType enum"
    );
};
