// CubeFaceData.hpp
//
// Pure data model for the "3D QR" tiled-peg pattern shown on a single
// outward-facing side of a cubie. This header has ZERO knowledge of 3D
// geometry, OpenGL, or anything rendering related -- it only describes
// which pegs are "on" or "off" for a given face.
//
// Layout:
//   A face is divided into a 3x3 array of tiles (9 tiles total).
//   Each tile holds a 4x4 array of pegs (16 pegs per tile).
//   That gives a 12x12 grid of peg bits overall (3*4 = 12 in each
//   dimension), stored as a flat std::bitset<144> where
//       bitIndex = row * 12 + col          (row, col in [0,12))
//   row/col are GLOBAL grid coordinates. To find which tile a bit
//   belongs to and which peg slot within that tile:
//       tileRow = row / 4   (0..2)
//       tileCol = col / 4   (0..2)
//       pegRow  = row % 4   (0..3)
//       pegCol  = col % 4   (0..3)
//   This mapping is intentionally simple (integer division/modulo) so a
//   tile with all-zero bits in its 4x4 sub-grid still exists (it is
//   rendered as a "blank" tile: rim + recessed inner floor, no pegs).

#pragma once

#include <bitset>
#include <cstddef>
#include <stdexcept>

namespace qrcube {

constexpr int kGridSize = 12;          // 12x12 bits total
constexpr int kTilesPerSide = 3;       // 3x3 tiles
constexpr int kPegsPerTileSide = 4;    // 4x4 pegs per tile
constexpr int kBitCount = kGridSize * kGridSize; // 144

// Holds the on/off peg pattern for ONE outward-facing side of ONE cubie.
// Generated once at startup per visible face and never mutated afterward
// by cube turns (turns only change a cubie's position/orientation, never
// its underlying pattern data).
class CubeFaceData {
public:
    CubeFaceData() = default;

    // Construct from an existing bit pattern (e.g. loaded/hardcoded).
    explicit CubeFaceData(const std::bitset<kBitCount>& bits) : bits_(bits) {}

    // Global-grid accessors -------------------------------------------------
    bool bitAt(int row, int col) const {
        checkBounds(row, col);
        return bits_[static_cast<std::size_t>(row * kGridSize + col)];
    }

    void setBit(int row, int col, bool value) {
        checkBounds(row, col);
        bits_[static_cast<std::size_t>(row * kGridSize + col)] = value;
    }

    // Tile/peg-local accessors ----------------------------------------------
    // tileRow, tileCol in [0,3); pegRow, pegCol in [0,4)
    bool pegAt(int tileRow, int tileCol, int pegRow, int pegCol) const {
        const int row = tileRow * kPegsPerTileSide + pegRow;
        const int col = tileCol * kPegsPerTileSide + pegCol;
        return bitAt(row, col);
    }

    void setPeg(int tileRow, int tileCol, int pegRow, int pegCol, bool value) {
        const int row = tileRow * kPegsPerTileSide + pegRow;
        const int col = tileCol * kPegsPerTileSide + pegCol;
        setBit(row, col, value);
    }

    const std::bitset<kBitCount>& rawBits() const { return bits_; }
    std::bitset<kBitCount>& rawBits() { return bits_; }

private:
    static void checkBounds(int row, int col) {
        if (row < 0 || row >= kGridSize || col < 0 || col >= kGridSize) {
            throw std::out_of_range("CubeFaceData: row/col out of [0,12) range");
        }
    }

    std::bitset<kBitCount> bits_; // default-constructed = all zero ("blank")
};

// ---------------------------------------------------------------------------
// Hardcoded pattern generators.
//
// This is where you would plug in real QR-code-derived bit data. For this
// project we ship a handful of simple procedural patterns so every visible
// cubie face has *some* distinct, recognizable peg pattern out of the box.
// See README.md "Customizing the pattern data" for how to swap these out.
// ---------------------------------------------------------------------------

// A deterministic pseudo-random pattern seeded by an integer, so each
// cubie face gets a different-looking but reproducible pattern.
inline CubeFaceData makePseudoRandomFaceData(unsigned int seed) {
    CubeFaceData data;
    unsigned int state = seed * 2654435761u + 1u; // simple xorshift-ish seed mix
    auto next = [&state]() -> unsigned int {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        return state;
    };
    for (int r = 0; r < kGridSize; ++r) {
        for (int c = 0; c < kGridSize; ++c) {
            const bool on = (next() & 1u) != 0u;
            data.setBit(r, c, on);
        }
    }
    return data;
}

// A simple ring/border pattern per tile (rim of pegs on, center off) --
// useful as a visually distinct "marker" pattern, QR-finder-square style.
inline CubeFaceData makeRingFaceData() {
    CubeFaceData data;
    for (int tr = 0; tr < kTilesPerSide; ++tr) {
        for (int tc = 0; tc < kTilesPerSide; ++tc) {
            for (int pr = 0; pr < kPegsPerTileSide; ++pr) {
                for (int pc = 0; pc < kPegsPerTileSide; ++pc) {
                    const bool onBorder =
                        (pr == 0 || pr == kPegsPerTileSide - 1 ||
                         pc == 0 || pc == kPegsPerTileSide - 1);
                    data.setPeg(tr, tc, pr, pc, onBorder);
                }
            }
        }
    }
    return data;
}

} // namespace qrcube
