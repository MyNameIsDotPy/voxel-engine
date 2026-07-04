#pragma once

#include "Voxel.h"

// ── GrassVoxel — base ─────────────────────────────────────────────────────────
// All grass variants share the same hardness and texture slots.
// Override color() to produce visual variety.
class GrassVoxel : public SolidVoxel {
public:
    float   hardness()      const override { return 0.6f; }
    uint8_t textureTop()    const override { return 0; } // grass_top
    uint8_t textureBottom() const override { return 2; } // dirt
    uint8_t textureSide()   const override { return 1; } // grass_side
};

// ── Variants ──────────────────────────────────────────────────────────────────

// Standard temperate grass — bright green
class GrassPlain : public GrassVoxel {
public:
    const char* name()  const override { return "grass_plain"; }
    glm::vec3   color() const override { return { 0.27f, 0.54f, 0.18f }; }
};

// Dry/arid grass — yellowed, sun-bleached
class GrassDry : public GrassVoxel {
public:
    const char* name()  const override { return "grass_dry"; }
    glm::vec3   color() const override { return { 0.65f, 0.58f, 0.20f }; }
};

// Snowy grass — white frosted tips
class GrassSnowy : public GrassVoxel {
public:
    const char* name()  const override { return "grass_snowy"; }
    glm::vec3   color() const override { return { 0.78f, 0.88f, 0.90f }; }
    uint8_t     textureTop() const override { return 11; } // snow_top (atlas slot)
};

// Dense / jungle grass — deep saturated green
class GrassDense : public GrassVoxel {
public:
    const char* name()  const override { return "grass_dense"; }
    glm::vec3   color() const override { return { 0.10f, 0.42f, 0.12f }; }
};
