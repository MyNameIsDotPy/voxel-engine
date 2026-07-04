#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>

// Orbital camera that revolves around a fixed target point.
// Controls:
//   Left-mouse drag  → orbit (yaw / pitch)
//   Scroll wheel     → zoom (distance)
class Camera {
public:
    float yaw      = 45.0f;   // degrees, horizontal angle
    float pitch    = 30.0f;   // degrees, vertical angle
    float distance = 5.0f;    // distance from target

    glm::vec3 target = glm::vec3(0.0f);

    static constexpr float MIN_DISTANCE =  1.0f;
    static constexpr float MAX_DISTANCE = 20.0f;
    static constexpr float MIN_PITCH    = -89.0f;
    static constexpr float MAX_PITCH    =  89.0f;

    // Call with mouse delta (pixels) while dragging
    void orbit(float deltaYaw, float deltaPitch) {
        yaw   += deltaYaw;
        pitch += deltaPitch;
        pitch  = std::clamp(pitch, MIN_PITCH, MAX_PITCH);
    }

    // Call with scroll delta (positive = zoom in)
    void zoom(float delta) {
        distance -= delta;
        distance  = std::clamp(distance, MIN_DISTANCE, MAX_DISTANCE);
    }

    // World-space position of the camera
    glm::vec3 getPosition() const {
        const float yRad = glm::radians(yaw);
        const float pRad = glm::radians(pitch);
        return target + glm::vec3(
            distance * std::cos(pRad) * std::cos(yRad),
            distance * std::sin(pRad),
            distance * std::cos(pRad) * std::sin(yRad)
        );
    }

    glm::mat4 getViewMatrix() const {
        return glm::lookAt(getPosition(), target, glm::vec3(0.0f, 1.0f, 0.0f));
    }
};
