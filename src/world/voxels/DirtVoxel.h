#pragma once

#include "Voxel.h"

// ── DirtVoxel ─────────────────────────────────────────────────────────────────
class DirtVoxel : public SolidVoxel {
public:
    const char* name()    const override { return "dirt"; }
    glm::vec3   color()   const override { return { 0.44f, 0.28f, 0.14f }; }
    float       hardness() const override { return 0.5f; }
    uint8_t textureTop()    const override { return 2; }
    uint8_t textureBottom() const override { return 2; }
    uint8_t textureSide()   const override { return 2; }
};
