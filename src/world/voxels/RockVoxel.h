#pragma once

#include "Voxel.h"

// ── RockVoxel — base ──────────────────────────────────────────────────────────
class RockVoxel : public SolidVoxel {
public:
    float hardness() const override { return 3.0f; }
};

// Standard smooth stone — medium grey
class RockSmooth : public RockVoxel {
public:
    const char* name()  const override { return "rock_smooth"; }
    glm::vec3   color() const override { return { 0.50f, 0.50f, 0.50f }; }
    uint8_t textureTop()    const override { return 3; }
    uint8_t textureBottom() const override { return 3; }
    uint8_t textureSide()   const override { return 3; }
};

// Cracked stone — darker, weathered
class RockCracked : public RockVoxel {
public:
    const char* name()  const override { return "rock_cracked"; }
    glm::vec3   color() const override { return { 0.34f, 0.32f, 0.30f }; }
    float       hardness() const override { return 2.5f; }
    uint8_t textureTop()    const override { return 13; }
    uint8_t textureBottom() const override { return 13; }
    uint8_t textureSide()   const override { return 13; }
};

// Mossy stone — grey-green tint
class RockMoss : public RockVoxel {
public:
    const char* name()  const override { return "rock_moss"; }
    glm::vec3   color() const override { return { 0.36f, 0.46f, 0.28f }; }
    float       hardness() const override { return 2.0f; }
    uint8_t textureTop()    const override { return 14; }
    uint8_t textureBottom() const override { return 14; }
    uint8_t textureSide()   const override { return 14; }
};
