#pragma once

#include <glm/glm.hpp>

// ── Voxel — abstract base ─────────────────────────────────────────────────────
// Every block type inherits from this.
// Chunks store BlockType (uint8_t) for memory; BlockRegistry maps that ID to
// a const Voxel* with the full behaviour and rendering info.
class Voxel {
public:
    virtual ~Voxel() = default;

    // Identity
    virtual const char* name() const = 0;

    // Physics / face-culling
    virtual bool isSolid()       const = 0;  // blocks movement & culls adjacent faces
    virtual bool isTransparent() const { return false; } // render faces next to this type
    virtual bool isLiquid()      const { return false; }

    // Rendering (used until TextureAtlas is ready)
    virtual glm::vec3 color() const = 0;  // base RGB color [0..1]
    virtual float     alpha() const { return 1.0f; } // 1 = opaque, <1 = transparent

    // Texture atlas slots — set when TextureAtlas (issue #4) is implemented
    virtual uint8_t textureTop()    const { return 0; }
    virtual uint8_t textureBottom() const { return 0; }
    virtual uint8_t textureSide()   const { return 0; }

    // Gameplay (future use)
    virtual float hardness()    const { return 1.0f; }  // seconds to break
    virtual int   lightLevel()  const { return  0;   }  // 0–15; lava = 15
};

// ── SolidVoxel — convenient base for all opaque solid blocks ──────────────────
class SolidVoxel : public Voxel {
public:
    bool isSolid()       const override { return true;  }
    bool isTransparent() const override { return false; }
};

// ── LiquidVoxel — base for Water, Lava ───────────────────────────────────────
class LiquidVoxel : public Voxel {
public:
    bool isSolid()       const override { return false; }
    bool isTransparent() const override { return true;  }
    bool isLiquid()      const override { return true;  }
    float hardness()     const override { return 0.0f;  }
};
