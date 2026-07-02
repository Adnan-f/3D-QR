#pragma once
// Pure binary data model: six faces, each a 12x12 bit grid, stored with
// std::bitset. This file has ZERO knowledge of 3D geometry -- it only
// knows about bits, rows/columns, tiles and pegs as abstract indices.
// A future automated decoder can populate a CubeData object via set()
// calls alone; Geometry.cpp/.h never need to change.

#include <array>
#include <bitset>
#include <cstddef>

namespace cube3d {

// ---- Grid resolution constants -------------------------------------
// The 12x12 per-face bit grid is partitioned into a 3x3 array of tiles,
// each tile holding a 4x4 array of pegs. The relationship between these
// constants is enforced below via static_assert so it can never silently
// drift out of sync.
constexpr int kGridResolution = 12;  // grid is kGridResolution x kGridResolution bits
constexpr int kTileCount = 3;        // kTileCount x kTileCount tiles per face
constexpr int kPegsPerTileSide = 4;  // kPegsPerTileSide x kPegsPerTileSide pegs per tile
constexpr int kBitsPerFace = kGridResolution * kGridResolution; // 144
constexpr int kFaceCount = 6;
constexpr int kTotalBits = kBitsPerFace * kFaceCount; // 864

static_assert(kTileCount * kPegsPerTileSide == kGridResolution,
              "tileCount * pegsPerTileSide must equal gridResolution");

// Identifies each of the six cube faces. Order matches the six local
// coordinate frames defined in Geometry.cpp.
enum class FaceId : int { PosZ = 0, NegZ, PosX, NegX, PosY, NegY };
constexpr int kFaceIdCount = static_cast<int>(FaceId::NegY) + 1;
static_assert(kFaceIdCount == kFaceCount, "FaceId enum must have exactly six entries");

// One face's 12x12 bit grid (144 bits), row-major indexing.
class FaceGrid {
public:
    // Row-major (row, col) accessors. row/col in [0, kGridResolution).
    bool get(int row, int col) const;
    void set(int row, int col, bool value);

    // Convenience accessor that maps directly to a peg within a tile:
    // tile (tileRow, tileCol) in [0, kTileCount), peg (pegRow, pegCol)
    // in [0, kPegsPerTileSide). This is exactly the inverse of the
    // "bit (row, col) maps to tile (row/4, col/4), peg (row%4, col%4)"
    // rule described in the project spec.
    bool pegAt(int tileRow, int tileCol, int pegRow, int pegCol) const;
    void setPeg(int tileRow, int tileCol, int pegRow, int pegCol, bool value);

    int countSetBits() const { return static_cast<int>(bits_.count()); }

private:
    static int flatIndex(int row, int col) { return row * kGridResolution + col; }
    std::bitset<kBitsPerFace> bits_;
};

// All six faces of the cube.
class CubeData {
public:
    FaceGrid& face(FaceId id) { return faces_[static_cast<size_t>(id)]; }
    const FaceGrid& face(FaceId id) const { return faces_[static_cast<size_t>(id)]; }

    int totalSetBits() const {
        int total = 0;
        for (const auto& f : faces_) total += f.countSetBits();
        return total;
    }

private:
    std::array<FaceGrid, kFaceCount> faces_;
};

} // namespace cube3d
