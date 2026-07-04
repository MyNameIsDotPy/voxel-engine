#pragma once

#include "BlockType.h"
#include "voxels/Voxel.h"
#include "voxels/GrassVoxel.h"
#include "voxels/DirtVoxel.h"
#include "voxels/RockVoxel.h"
#include "voxels/SnowVoxel.h"
#include "voxels/WaterVoxel.h"
#include "voxels/LavaVoxel.h"

#include <cassert>
#include <memory>
#include <array>

// ── AirVoxel (internal) ───────────────────────────────────────────────────────
class AirVoxel : public Voxel {
public:
    const char* name()         const override { return "air";          }
    bool        isSolid()      const override { return false;          }
    bool        isTransparent() const override { return true;          }
    glm::vec3   color()        const override { return { 0,0,0 };      }
};

// ── BlockRegistry ─────────────────────────────────────────────────────────────
// Maps BlockType → const Voxel&.
// All instances are statically allocated — no heap allocation at runtime.
class BlockRegistry {
public:
    static const Voxel& get(BlockType type) {
        const int idx = static_cast<int>(type);
        assert(idx >= 0 && idx < static_cast<int>(BlockType::COUNT));
        return *s_voxels[idx];
    }

private:
    // Order must match BlockType enum exactly
    inline static const Voxel* const s_voxels[] = {
        new AirVoxel(),     // Air
        new GrassPlain(),   // GrassPlain
        new GrassDry(),     // GrassDry
        new GrassSnowy(),   // GrassSnowy
        new GrassDense(),   // GrassDense
        new DirtVoxel(),    // Dirt
        new RockSmooth(),   // RockSmooth
        new RockCracked(),  // RockCracked
        new RockMoss(),     // RockMoss
        new SnowVoxel(),    // Snow
        new WaterVoxel(),   // Water
        new LavaVoxel(),    // Lava
    };

    static_assert(
        sizeof(s_voxels) / sizeof(s_voxels[0]) == static_cast<int>(BlockType::COUNT),
        "BlockRegistry::s_voxels is out of sync with BlockType enum"
    );
};
