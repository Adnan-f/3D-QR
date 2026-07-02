// CubieMeshGenerator.hpp
//
// Builds GPU-ready geometry (interleaved position+normal vertices, plus a
// triangle index buffer) for ONE cubie, given:
//   - which of its 6 local faces (+X,-X,+Y,-Y,+Z,-Z) are externally visible
//     on the 3x3x3 arrangement, and
//   - the CubeFaceData (12x12 peg bits) to use for each visible face.
//
// Geometric construction summary (see CubieMeshGenerator.cpp for the full
// derivation in comments):
//
//   Each visible face has a local 2D frame: an origin O at the face's
//   center, two in-plane axes U and V, and an outward normal N, with
//   cross(U, V) == N (verified per-face in the .cpp file). A point with
//   local coordinates (u, v, h) -- h measured along N -- maps to a 3D
//   position via:
//       worldPos = O + u*U + v*V + h*N
//   This single formula is reused, unmodified, for every feature on every
//   face: the base panel, every tile's walls/rim/floor, and every peg's
//   walls/cap. No face gets special-cased construction code; only the
//   (O, U, V, N) frame differs per face.
//
//   Height levels along N:
//     h = 0                      -> base panel (recessed background)
//     h = tileRaiseHeight        -> top of each tile's rim
//     h = tileFloorHeight        -> each tile's recessed inner floor
//                                    (tileFloorHeight < tileRaiseHeight)
//     h = tileFloorHeight+pegHeight -> top of each "on" peg
//
// Interior (non-visible) faces are emitted as a single flat quad at h=0
// with the face's outward normal -- cheap placeholder geometry that is
// never seen from outside the assembled puzzle.

#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include <glm/glm.hpp>

#include "CubeFaceData.hpp"

namespace qrcube {

// One interleaved vertex: position + flat (per-triangle) normal.
struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
};

// Identifies the 6 local faces of a cubie in a fixed, documented order.
enum class FaceId : int { PosX = 0, NegX = 1, PosY = 2, NegY = 3, PosZ = 4, NegZ = 5 };
constexpr int kFaceCount = 6;

// Tunable geometric proportions for the tiled-peg pattern. All distances
// are expressed relative to cubieSize (the edge length of one cubie's
// bounding cube), so the look stays consistent regardless of scale.
struct CubieMeshParams {
    float cubieSize = 1.0f;          // edge length of the cubie's bounding box
    float faceMargin = 0.04f;        // margin between cubie edge and tile array, in cubieSize units
    float tileGap = 0.012f;          // gap between adjacent tiles, in cubieSize units
    float tileRaiseHeight = 0.05f;   // tile rim top height above base panel
    float rimWidth = 0.018f;         // width of each tile's raised rim/frame
    float floorRecess = 0.018f;      // how far the inner floor sits below the rim top
    float pegGap = 0.006f;           // gap between adjacent pegs within a tile
    float pegHeight = 0.035f;        // peg height above the tile's inner floor
};

// Result of generating one cubie's full mesh.
struct CubieMesh {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
};

// Per-face frame: origin, two in-plane axes, and outward normal, all in the
// cubie's own local (object) space, where the cubie spans
// [-cubieSize/2, +cubieSize/2] along each axis.
struct FaceFrame {
    glm::vec3 origin;
    glm::vec3 uAxis;
    glm::vec3 vAxis;
    glm::vec3 normal;
};

// Returns the fixed, verified set of 6 face frames for a cubie of the given
// size, centered at the local origin. See the .cpp file for the
// cross(uAxis, vAxis) == normal verification for each entry.
std::array<FaceFrame, kFaceCount> makeCubieFaceFrames(float cubieSize);

// Generates the full mesh for one cubie.
//   visibleFaces[i]  -- true if FaceId(i) is externally visible and should
//                       get the tiled-peg pattern.
//   faceData[i]      -- the 12x12 peg pattern to use for FaceId(i). Only
//                       read when visibleFaces[i] is true.
CubieMesh generateCubieMesh(const CubieMeshParams& params,
                             const std::array<bool, kFaceCount>& visibleFaces,
                             const std::array<CubeFaceData, kFaceCount>& faceData);

} // namespace qrcube
