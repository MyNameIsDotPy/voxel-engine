#pragma once

#include "BlockRegistry.h"
#include "World.h"

#include <glm/glm.hpp>

#include <cmath>
#include <limits>

struct RayHit {
    bool hit = false;
    glm::ivec3 block{0};
    glm::ivec3 normal{0};
    glm::vec3 position{0.0f};
    BlockType type = BlockType::Air;
    float distance = 0.0f;

    static RayHit miss() {
        return {};
    }
};

class RayCast {
public:
    static RayHit cast(glm::vec3 origin, glm::vec3 dir, float maxDist, World& world) {
        if (glm::dot(dir, dir) == 0.0f || maxDist <= 0.0f) {
            return RayHit::miss();
        }

        dir = glm::normalize(dir);
        glm::ivec3 voxel((int)std::floor(origin.x),
                         (int)std::floor(origin.y),
                         (int)std::floor(origin.z));

        const glm::ivec3 step(dir.x > 0.0f ? 1 : -1,
                              dir.y > 0.0f ? 1 : -1,
                              dir.z > 0.0f ? 1 : -1);

        const glm::vec3 tDelta(dir.x == 0.0f ? inf() : std::abs(1.0f / dir.x),
                               dir.y == 0.0f ? inf() : std::abs(1.0f / dir.y),
                               dir.z == 0.0f ? inf() : std::abs(1.0f / dir.z));

        glm::vec3 tMax(initialTMax(origin.x, dir.x, voxel.x),
                       initialTMax(origin.y, dir.y, voxel.y),
                       initialTMax(origin.z, dir.z, voxel.z));

        glm::ivec3 normal(0);
        float dist = 0.0f;

        while (dist <= maxDist) {
            const BlockType type = world.getBlock(voxel);
            if (BlockRegistry::get(type).isSolid()) {
                return {true, voxel, normal, origin + dir * dist, type, dist};
            }

            if (tMax.x < tMax.y && tMax.x < tMax.z) {
                voxel.x += step.x;
                dist = tMax.x;
                tMax.x += tDelta.x;
                normal = {-step.x, 0, 0};
            } else if (tMax.y < tMax.z) {
                voxel.y += step.y;
                dist = tMax.y;
                tMax.y += tDelta.y;
                normal = {0, -step.y, 0};
            } else {
                voxel.z += step.z;
                dist = tMax.z;
                tMax.z += tDelta.z;
                normal = {0, 0, -step.z};
            }
        }

        return RayHit::miss();
    }

private:
    static float inf() {
        return std::numeric_limits<float>::infinity();
    }

    static float initialTMax(float origin, float dir, int voxel) {
        if (dir > 0.0f) {
            return ((float)voxel + 1.0f - origin) / dir;
        }
        if (dir < 0.0f) {
            return (origin - (float)voxel) / -dir;
        }
        return inf();
    }
};
