#pragma once

#include "World.h"

#include <glm/glm.hpp>

#include <algorithm>
#include <cmath>

class Player;

class Physics {
public:
    static void resolveAABB(Player& player, World& world, float dt);

private:
    static bool overlapsSolid(const glm::vec3& center, const glm::vec3& halfExtents, const World& world) {
        const glm::vec3 mn = center - halfExtents;
        const glm::vec3 mx = center + halfExtents;

        const glm::ivec3 minBlock((int)std::floor(mn.x),
                                  (int)std::floor(mn.y),
                                  (int)std::floor(mn.z));
        const glm::ivec3 maxBlock((int)std::floor(mx.x - 0.0001f),
                                  (int)std::floor(mx.y - 0.0001f),
                                  (int)std::floor(mx.z - 0.0001f));

        for (int y = minBlock.y; y <= maxBlock.y; ++y)
        for (int z = minBlock.z; z <= maxBlock.z; ++z)
        for (int x = minBlock.x; x <= maxBlock.x; ++x) {
            if (world.isSolid({x, y, z})) {
                return true;
            }
        }
        return false;
    }

    static bool moveAxis(glm::vec3& position,
                         const glm::vec3& halfExtents,
                         const World& world,
                         int axis,
                         float delta,
                         bool& hitGround) {
        if (delta == 0.0f) {
            return false;
        }

        const float sign = delta > 0.0f ? 1.0f : -1.0f;
        float remaining = std::abs(delta);
        constexpr float stepSize = 0.05f;

        while (remaining > 0.0f) {
            const float step = std::min(stepSize, remaining) * sign;
            glm::vec3 next = position;
            next[axis] += step;

            if (overlapsSolid(next, halfExtents, world)) {
                if (axis == 1 && sign < 0.0f) {
                    hitGround = true;
                }
                return true;
            }

            position = next;
            remaining -= std::abs(step);
        }

        return false;
    }
};

