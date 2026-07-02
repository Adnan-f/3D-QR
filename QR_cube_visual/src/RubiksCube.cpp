// RubiksCube.cpp
#include "RubiksCube.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <unordered_map>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/quaternion.hpp>

namespace qrcube {

namespace {
const std::unordered_map<std::string, TurnRequest>& moveTable() {
    static const std::unordered_map<std::string, TurnRequest> table = {
        {"U",  {Axis::Y, +1, -1}},
        {"U'", {Axis::Y, +1, +1}},
        {"D",  {Axis::Y, -1, +1}},
        {"D'", {Axis::Y, -1, -1}},
        {"R",  {Axis::X, +1, -1}},
        {"R'", {Axis::X, +1, +1}},
        {"L",  {Axis::X, -1, +1}},
        {"L'", {Axis::X, -1, -1}},
        {"F",  {Axis::Z, +1, -1}},
        {"F'", {Axis::Z, +1, +1}},
        {"B",  {Axis::Z, -1, +1}},
        {"B'", {Axis::Z, -1, -1}},
    };
    return table;
}
} // namespace

std::optional<TurnRequest> lookupMove(const std::string& moveName) {
    const auto& table = moveTable();
    auto it = table.find(moveName);
    if (it == table.end()) return std::nullopt;
    return it->second;
}

glm::vec3 RubiksCube::axisVector(Axis axis) {
    switch (axis) {
        case Axis::X: return glm::vec3(1, 0, 0);
        case Axis::Y: return glm::vec3(0, 1, 0);
        case Axis::Z: return glm::vec3(0, 0, 1);
    }
    return glm::vec3(0, 1, 0);
}

RubiksCube::RubiksCube(std::vector<Cubie> cubies) : cubies_(std::move(cubies)) {
    if (cubies_.size() != 27) {
        throw std::runtime_error("RubiksCube requires exactly 27 cubies");
    }
    rebuildSlotMapping();
}

void RubiksCube::rebuildSlotMapping() {
    for (auto& plane : slotToCubie_)
        for (auto& row : plane)
            for (auto& v : row) v = -1;

    for (int i = 0; i < static_cast<int>(cubies_.size()); ++i) {
        glm::ivec3 p = cubies_[static_cast<size_t>(i)].gridPos();
        int sx = slotIndex(p.x), sy = slotIndex(p.y), sz = slotIndex(p.z);
        if (sx < 0 || sx > 2 || sy < 0 || sy > 2 || sz < 0 || sz > 2) {
            throw std::runtime_error("RubiksCube: cubie position out of [-1,1]^3 range");
        }
        if (slotToCubie_[static_cast<size_t>(sx)][static_cast<size_t>(sy)][static_cast<size_t>(sz)] != -1) {
            throw std::runtime_error("RubiksCube: duplicate cubie slot detected during mapping rebuild");
        }
        slotToCubie_[static_cast<size_t>(sx)][static_cast<size_t>(sy)][static_cast<size_t>(sz)] = i;
    }
    // Verify every slot is filled exactly once.
    for (auto& plane : slotToCubie_)
        for (auto& row : plane)
            for (auto& v : row)
                if (v == -1) throw std::runtime_error("RubiksCube: empty slot detected during mapping rebuild");
}

int RubiksCube::cubieIndexAtSlot(glm::ivec3 slot) const {
    int sx = slotIndex(slot.x), sy = slotIndex(slot.y), sz = slotIndex(slot.z);
    if (sx < 0 || sx > 2 || sy < 0 || sy > 2 || sz < 0 || sz > 2) return -1;
    return slotToCubie_[static_cast<size_t>(sx)][static_cast<size_t>(sy)][static_cast<size_t>(sz)];
}

bool RubiksCube::requestTurn(const TurnRequest& req) {
    if (activeTurn_.has_value()) {
        // A turn is already animating: ignore additional input (documented
        // policy in the header / Part 5 input docs).
        return false;
    }

    ActiveTurn at;
    at.request = req;
    at.elapsed = 0.0f;

    int axisIdx = (req.axis == Axis::X) ? 0 : (req.axis == Axis::Y ? 1 : 2);
    for (int i = 0; i < static_cast<int>(cubies_.size()); ++i) {
        glm::ivec3 p = cubies_[static_cast<size_t>(i)].gridPos();
        int coord = (axisIdx == 0) ? p.x : (axisIdx == 1 ? p.y : p.z);
        if (coord == req.layer) {
            at.cubieIndices.push_back(i);
        }
    }
    // Sanity: every face/middle layer of a 3x3x3 always has exactly 9 cubies.
    if (at.cubieIndices.size() != 9) {
        throw std::runtime_error("RubiksCube::requestTurn: selected layer did not contain exactly 9 cubies");
    }

    activeTurn_ = std::move(at);
    return true;
}

void RubiksCube::applyTurnImmediate(const TurnRequest& req) {
    int axisIdx = (req.axis == Axis::X) ? 0 : (req.axis == Axis::Y ? 1 : 2);
    glm::vec3 axisVec = axisVector(req.axis);
    glm::quat rot = glm::angleAxis(static_cast<float>(req.sign) * glm::half_pi<float>(), axisVec);
    glm::mat3 rotMat = glm::mat3_cast(rot);

    std::vector<int> selected;
    for (int i = 0; i < static_cast<int>(cubies_.size()); ++i) {
        glm::ivec3 p = cubies_[static_cast<size_t>(i)].gridPos();
        int coord = (axisIdx == 0) ? p.x : (axisIdx == 1 ? p.y : p.z);
        if (coord == req.layer) selected.push_back(i);
    }
    if (selected.size() != 9) {
        throw std::runtime_error("RubiksCube::applyTurnImmediate: selected layer did not contain exactly 9 cubies");
    }

    // Compute new positions/orientations first (reading old gridPos for
    // all selected cubies) before writing any of them, so we never read a
    // partially-updated state mid-loop.
    std::vector<glm::ivec3> newPositions(selected.size());
    std::vector<glm::quat> newOrientations(selected.size());
    for (size_t k = 0; k < selected.size(); ++k) {
        Cubie& c = cubies_[static_cast<size_t>(selected[k])];
        glm::vec3 fp = rotMat * glm::vec3(c.gridPos());
        glm::ivec3 newPos(static_cast<int>(std::round(fp.x)),
                           static_cast<int>(std::round(fp.y)),
                           static_cast<int>(std::round(fp.z)));
        newPositions[k] = newPos;
        newOrientations[k] = glm::normalize(rot * c.orientation()); // left-multiply
    }

    for (size_t k = 0; k < selected.size(); ++k) {
        Cubie& c = cubies_[static_cast<size_t>(selected[k])];
        c.setGridPos(newPositions[k]);
        c.setOrientation(newOrientations[k]);
    }

    rebuildSlotMapping(); // also verifies no duplicate/empty slots resulted
}

void RubiksCube::update(float dt) {
    if (!activeTurn_.has_value()) return;

    activeTurn_->elapsed += dt;
    if (activeTurn_->elapsed >= kTurnDuration) {
        // Commit: apply the real grid-position/orientation change now.
        TurnRequest req = activeTurn_->request;
        activeTurn_.reset(); // clear animation state before mutating, so
                              // computeModelMatrix() during applyTurnImmediate
                              // (if ever called) doesn't double-apply.
        applyTurnImmediate(req);
    }
}

glm::mat4 RubiksCube::computeModelMatrix(int index) const {
    const Cubie& c = cubies_[static_cast<size_t>(index)];
    glm::mat4 base = c.baseModelMatrix(kCubieSpacing);

    if (!activeTurn_.has_value()) return base;

    const ActiveTurn& at = *activeTurn_;
    bool participating = std::find(at.cubieIndices.begin(), at.cubieIndices.end(), index) != at.cubieIndices.end();
    if (!participating) return base;

    float t = std::clamp(at.elapsed / kTurnDuration, 0.0f, 1.0f);
    float angle = static_cast<float>(at.request.sign) * glm::half_pi<float>() * t;
    glm::mat4 extra = glm::rotate(glm::mat4(1.0f), angle, axisVector(at.request.axis));

    // The extra rotation is about the cube's central axis (world/object
    // origin), so it must be applied OUTSIDE (left-multiplied onto) the
    // cubie's own base model matrix -- this makes the whole layer swing
    // together as a rigid group around the shared axis, rather than each
    // cubie spinning in place about its own center.
    return extra * base;
}

} // namespace qrcube
