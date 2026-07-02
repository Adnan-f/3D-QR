// Camera.cpp
#include "Camera.hpp"

#include <algorithm>
#include <cmath>

#include <glm/gtc/matrix_transform.hpp>

namespace qrcube {

void Camera::orbit(float deltaYawDegrees, float deltaPitchDegrees) {
    yawDegrees += deltaYawDegrees;
    pitchDegrees = std::clamp(pitchDegrees + deltaPitchDegrees, minPitch, maxPitch);
}

void Camera::zoom(float scrollDelta) {
    // Scroll up (positive) zooms in (reduces distance).
    distance = std::clamp(distance - scrollDelta, minDistance, maxDistance);
}

glm::vec3 Camera::position() const {
    float yaw = glm::radians(yawDegrees);
    float pitch = glm::radians(pitchDegrees);
    glm::vec3 offset;
    offset.x = distance * std::cos(pitch) * std::sin(yaw);
    offset.y = distance * std::sin(pitch);
    offset.z = distance * std::cos(pitch) * std::cos(yaw);
    return target + offset;
}

glm::mat4 Camera::viewMatrix() const {
    return glm::lookAt(position(), target, glm::vec3(0.0f, 1.0f, 0.0f));
}

glm::mat4 Camera::projectionMatrix(float aspectRatio) const {
    return glm::perspective(glm::radians(fovYDegrees), aspectRatio, nearPlane, farPlane);
}

} // namespace qrcube
