// RubiksCube.hpp
//
// Owns all 27 Cubies, the position->cubie mapping, face-turn logic, and
// turn animation state.
//
// MOVE NOTATION TABLE (axis / layer / sign), right-handed local axes
// X=right, Y=up, Z=towards viewer. "sign" is the rotation sign used with
// glm::angleAxis(sign * 90deg, axisVector) under the standard
// right-hand rule (positive angle = counter-clockwise when looking from
// the positive axis end back toward the origin).
//
//   Move | axis | layer | sign | meaning
//   -----+------+-------+------+----------------------------------
//   U    |  Y   |  +1   |  -1  | top layer,    clockwise from above
//   U'   |  Y   |  +1   |  +1  | top layer,    counter-clockwise
//   D    |  Y   |  -1   |  +1  | bottom layer, clockwise from below
//   D'   |  Y   |  -1   |  -1  | bottom layer, counter-clockwise
//   R    |  X   |  +1   |  -1  | right layer,  clockwise from the right
//   R'   |  X   |  +1   |  +1  | right layer,  counter-clockwise
//   L    |  X   |  -1   |  +1  | left layer,   clockwise from the left
//   L'   |  X   |  -1   |  -1  | left layer,   counter-clockwise
//   F    |  Z   |  +1   |  -1  | front layer,  clockwise facing the cube
//   F'   |  Z   |  +1   |  +1  | front layer,  counter-clockwise
//   B    |  Z   |  -1   |  +1  | back layer,   clockwise from the back
//   B'   |  Z   |  -1   |  -1  | back layer,   counter-clockwise
//
// (This is a self-consistent convention -- "clockwise" here means
// clockwise as seen by an observer looking at that face from outside the
// cube, which is the standard informal Rubik's-cube definition. What
// matters for correctness, and what is explicitly verified in
// RubiksCubeTest, is that 4 identical turns return to the identity and
// that each turn touches exactly the intended 9 cubies.)
//
// ANIMATION / COMMIT MODEL (Part 4):
// We use the "commit on completion" approach: starting a turn does NOT
// immediately change any cubie's gridPos/orientation. Instead it snapshots
// which 9 cubies are turning and animates a purely-visual extra rotation
// (about the cube's central axis) on top of their existing committed
// model matrices, interpolated from 0 to 90 degrees over kTurnDuration
// seconds. Only when the animation reaches completion do we actually
// rotate gridPos/orientation and update the slot mapping. While a turn is
// animating, further turn requests are ignored (see requestTurn()).

#pragma once

#include <array>
#include <functional>
#include <optional>
#include <string>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "Cubie.hpp"

namespace qrcube {

enum class Axis { X, Y, Z };

struct TurnRequest {
    Axis axis;
    int layer;  // -1, 0, or +1
    int sign;   // -1 or +1
};

// Looks up the (axis, layer, sign) triple for one of the 12 standard moves.
// moveName must be one of: "U","U'","D","D'","L","L'","R","R'","F","F'","B","B'"
std::optional<TurnRequest> lookupMove(const std::string& moveName);

class RubiksCube {
public:
    static constexpr float kCubieSpacing = 1.05f; // gap between cubie centers
    static constexpr float kTurnDuration = 0.25f; // seconds per 90-degree turn

    // cubies must already be filled with 27 entries at their initial grid
    // positions (one for each of {-1,0,1}^3), with identity orientation.
    explicit RubiksCube(std::vector<Cubie> cubies);

    // Begins animating the given turn. If a turn is already animating, this
    // call is ignored (documented "ignore additional input" policy).
    // Returns true if the turn was actually started.
    bool requestTurn(const TurnRequest& req);

    // Advances animation state by dt seconds. If an animation completes
    // during this call, commits the grid-position/orientation changes and
    // updates the slot mapping.
    void update(float dt);

    bool isAnimating() const { return activeTurn_.has_value(); }

    int cubieCount() const { return static_cast<int>(cubies_.size()); }
    const Cubie& cubieAt(int index) const { return cubies_[static_cast<size_t>(index)]; }

    // Computes the full render model matrix for cubie `index`, including
    // any in-progress turn animation contribution.
    glm::mat4 computeModelMatrix(int index) const;

    // --- Verification / testing helpers (also used by RubiksCubeTest) ---
    glm::ivec3 mappedPosition(int cubieIndex) const { return cubies_[static_cast<size_t>(cubieIndex)].gridPos(); }
    int cubieIndexAtSlot(glm::ivec3 slot) const;

    // Applies a turn's grid-position/orientation change to all cubies in
    // the given layer IMMEDIATELY (no animation). Used internally by
    // update() on animation completion, and directly by RubiksCubeTest to
    // verify the underlying algebra in isolation from any rendering/timing
    // concerns.
    void applyTurnImmediate(const TurnRequest& req);

private:
    std::vector<Cubie> cubies_;
    // slot (offset by +1 so components are in [0,3)) -> index into cubies_
    std::array<std::array<std::array<int, 3>, 3>, 3> slotToCubie_{};

    struct ActiveTurn {
        TurnRequest request;
        std::vector<int> cubieIndices; // indices into cubies_ participating
        float elapsed = 0.0f;
    };
    std::optional<ActiveTurn> activeTurn_;

    void rebuildSlotMapping();
    static glm::vec3 axisVector(Axis axis);
    static int slotIndex(int gridCoord) { return gridCoord + 1; } // -1,0,1 -> 0,1,2
};

} // namespace qrcube
