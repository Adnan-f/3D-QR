// Camera.hpp
// A simple orbit camera: orbits around a fixed target (the cube's center)
// at a configurable distance, controlled by yaw/pitch angles and a zoom
// (distance) value. Mouse-drag updates yaw/pitch; scroll updates distance.

#pragma once

#include <glm/glm.hpp>

namespace qrcube {

class Camera {
public:
    Camera() = default;

    // Call once per drag-delta (in pixels or any consistent unit).
    void orbit(float deltaYawDegrees, float deltaPitchDegrees);

    // Call once per scroll event. Positive = zoom in.
    void zoom(float scrollDelta);

    glm::mat4 viewMatrix() const;
    glm::mat4 projectionMatrix(float aspectRatio) const;
    glm::vec3 position() const;

    glm::vec3 target = glm::vec3(0.0f, 0.0f, 0.0f);
    float distance = 8.0f;
    float yawDegrees = 35.0f;     // rotation around the world Y axis
    float pitchDegrees = 25.0f;   // rotation up/down
    float fovYDegrees = 45.0f;
    float nearPlane = 0.1f;
    float farPlane = 100.0f;

    float minDistance = 3.0f;
    float maxDistance = 25.0f;
    float minPitch = -89.0f;
    float maxPitch = 89.0f;
};

} // namespace qrcube
