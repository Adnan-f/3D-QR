// Cubie.hpp
// Represents one of the 27 small cube pieces: its current grid slot,
// accumulated orientation, and a reference to its (fixed, pre-generated)
// mesh range within the shared combined vertex/index buffers.

#pragma once

#include <cstdint>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace qrcube {

// Describes where in the single shared VBO/EBO this cubie's geometry lives.
// All 27 cubies' meshes are generated once and concatenated into one big
// buffer; each cubie just remembers its slice.
struct MeshRange {
    uint32_t indexOffsetBytes = 0; // byte offset into the shared EBO
    uint32_t indexCount = 0;       // number of indices for this cubie
    int32_t baseVertex = 0;        // base vertex for glDrawElementsBaseVertex
};

class Cubie {
public:
    Cubie() = default;
    Cubie(glm::ivec3 initialGridPos, MeshRange range)
        : gridPos_(initialGridPos), meshRange_(range) {}

    glm::ivec3 gridPos() const { return gridPos_; }
    void setGridPos(glm::ivec3 p) { gridPos_ = p; }

    const glm::quat& orientation() const { return orientation_; }
    void setOrientation(const glm::quat& q) { orientation_ = q; }

    const MeshRange& meshRange() const { return meshRange_; }

    // Base model matrix from this cubie's *committed* state (no in-progress
    // turn animation applied): translate to its grid slot (scaled by
    // cubieSpacing) and rotate by its accumulated orientation.
    glm::mat4 baseModelMatrix(float cubieSpacing) const {
        glm::mat4 t = glm::translate(glm::mat4(1.0f), glm::vec3(gridPos_) * cubieSpacing);
        glm::mat4 r = glm::mat4_cast(orientation_);
        return t * r;
    }

private:
    glm::ivec3 gridPos_ = glm::ivec3(0, 0, 0);
    glm::quat orientation_ = glm::quat(1.0f, 0.0f, 0.0f, 0.0f); // identity
    MeshRange meshRange_;
};

} // namespace qrcube
