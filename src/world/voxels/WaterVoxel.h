#pragma once

#include "Voxel.h"

// ── WaterVoxel ────────────────────────────────────────────────────────────────
// Transparent liquid — faces adjacent to water are still rendered.
// Render pass: after all opaque blocks, back-to-front order (future).
class WaterVoxel : public LiquidVoxel {
public:
    const char* name()    const override { return "water"; }
    glm::vec3   color()   const override { return { 0.15f, 0.42f, 0.80f }; }
    float       alpha()   const override { return 0.55f; }
    uint8_t textureTop()    const override { return 6; }
    uint8_t textureBottom() const override { return 6; }
    uint8_t textureSide()   const override { return 6; }
};
