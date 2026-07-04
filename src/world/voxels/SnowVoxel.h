#pragma once

#include "Voxel.h"

// ── SnowVoxel ─────────────────────────────────────────────────────────────────
class SnowVoxel : public SolidVoxel {
public:
    const char* name()    const override { return "snow"; }
    glm::vec3   color()   const override { return { 0.92f, 0.95f, 1.00f }; }
    float       hardness() const override { return 0.2f; }
    uint8_t textureTop()    const override { return 11; }
    uint8_t textureBottom() const override { return 11; }
    uint8_t textureSide()   const override { return 11; }
};
