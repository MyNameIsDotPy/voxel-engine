#pragma once

#include "Voxel.h"

// ── LavaVoxel ─────────────────────────────────────────────────────────────────
// Opaque liquid — emits maximum light level (15).
// Does not cull adjacent faces (isTransparent = true for face visibility),
// but is not visually see-through.
class LavaVoxel : public LiquidVoxel {
public:
    const char* name()         const override { return "lava"; }
    glm::vec3   color()        const override { return { 0.90f, 0.30f, 0.04f }; }
    bool        isTransparent() const override { return false; } // opaque liquid
    int         lightLevel()   const override { return 15; }
    uint8_t textureTop()    const override { return 15; }
    uint8_t textureBottom() const override { return 15; }
    uint8_t textureSide()   const override { return 15; }
};
