#pragma once

#include "Voxel.h"

class SandVoxel : public SolidVoxel {
public:
    const char* name()     const override { return "sand"; }
    glm::vec3   color()    const override { return { 0.76f, 0.70f, 0.50f }; }
    float       hardness() const override { return 0.5f; }
    uint8_t textureTop()    const override { return 4; }
    uint8_t textureBottom() const override { return 4; }
    uint8_t textureSide()   const override { return 4; }
};
