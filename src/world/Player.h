#pragma once

#include "Physics.h"
#include "World.h"

#include <glm/glm.hpp>
#include <algorithm>

struct PlayerInput {
    bool forward  = false;
    bool backward = false;
    bool left     = false;
    bool right    = false;
    bool jump     = false;  // Space  — jump on ground, swim up in water, fly up in noclip
    bool down     = false;  // LCtrl  — fly down in noclip
    bool sprint   = false;
};

class Player {
public:
    glm::vec3 position    { 8.0f, 28.0f, 8.0f };
    glm::vec3 velocity    { 0.0f };
    glm::vec3 halfExtents { 0.3f, 0.9f, 0.3f };
    bool      onGround    = false;
    bool      noclip      = false;

    float walkSpeed   =  5.0f;
    float sprintSpeed =  8.0f;
    float jumpSpeed   =  7.5f;
    float gravity     = -24.0f;

    glm::vec3 min()    const { return position - halfExtents; }
    glm::vec3 max()    const { return position + halfExtents; }
    AABB      aabb()   const { return { min(), max() }; }
    glm::vec3 eyePos() const { return position + glm::vec3(0.0f, 0.72f, 0.0f); }

    bool isInWater(const World& world) const {
        // Check if the block at the player's mid-body is water
        const glm::ivec3 bp(
            (int)std::floor(position.x),
            (int)std::floor(position.y),
            (int)std::floor(position.z)
        );
        return world.getBlock(bp) == BlockType::Water;
    }

    void handleInput(const PlayerInput& input,
                     const glm::vec3& forward,
                     const glm::vec3& right,
                     const World& world)
    {
        glm::vec3 wishDir(0.0f);
        if (input.forward)  wishDir += forward;
        if (input.backward) wishDir -= forward;
        if (input.right)    wishDir += right;
        if (input.left)     wishDir -= right;

        wishDir.y = 0.0f;
        if (glm::dot(wishDir, wishDir) > 0.0f)
            wishDir = glm::normalize(wishDir);

        const float speed = input.sprint ? sprintSpeed : walkSpeed;

        if (noclip) {
            // ── Noclip: free 3D flight ────────────────────────────────────
            const float flySpeed = input.sprint ? sprintSpeed * 2.5f : sprintSpeed;
            velocity = wishDir * flySpeed;
            velocity.y = 0.0f;
            if (input.jump) velocity.y =  flySpeed;
            if (input.down) velocity.y = -flySpeed;
            return;
        }

        const bool swimming = isInWater(world);

        if (swimming) {
            // ── Swim: reduced speed, Space = swim up ──────────────────────
            velocity.x = wishDir.x * speed * 0.55f;
            velocity.z = wishDir.z * speed * 0.55f;
            if (input.jump) velocity.y =  3.5f;   // swim upward
        } else {
            // ── Normal ground movement ────────────────────────────────────
            velocity.x = wishDir.x * speed;
            velocity.z = wishDir.z * speed;
            if (onGround && input.jump) {
                velocity.y = jumpSpeed;
                onGround   = false;
            }
        }
    }

    void update(float dt, World& world) {
        if (noclip) {
            position += velocity * dt;
            onGround  = false;
            return;
        }

        const bool swimming = isInWater(world);

        if (swimming) {
            // Buoyancy: reduced effective gravity + terminal velocity cap
            const float effectiveGravity = gravity * 0.18f;
            velocity.y += effectiveGravity * dt;
            velocity.y  = std::clamp(velocity.y, -3.0f, 4.0f);
        } else {
            velocity.y += gravity * dt;
        }

        Physics::resolveAABB(*this, world, dt);
    }
};

inline void Physics::resolveAABB(Player& player, World& world, float dt) {
    world.update(player.position);

    player.onGround = false;
    bool hitGround  = false;

    const float dx = player.velocity.x * dt;
    const float dz = player.velocity.z * dt;
    const float dy = player.velocity.y * dt;

    if (moveAxis(player.position, player.halfExtents, world, 0, dx, hitGround))
        player.velocity.x = 0.0f;
    if (moveAxis(player.position, player.halfExtents, world, 2, dz, hitGround))
        player.velocity.z = 0.0f;
    if (moveAxis(player.position, player.halfExtents, world, 1, dy, hitGround))
        player.velocity.y = 0.0f;

    player.onGround = hitGround;
}
