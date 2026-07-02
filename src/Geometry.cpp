#include "Geometry.h"

#include <array>
#include <cassert>
#include <cmath>

namespace cube3d {
namespace {

// ---------------------------------------------------------------------
// Face local coordinate frames
// ---------------------------------------------------------------------
// Each face is described by an outward unit normal `n` and two in-plane
// unit axes `u` (maps to local "s" coordinate) and `v` (maps to local
// "t" coordinate). A point at in-plane position (s, t) and height h
// above the face's nominal (base-panel) level is:
//
//     P(s, t, h) = n * (half + h) + u * s + v * t
//
// where `half = cubeSize / 2`. This is the ONLY place any per-face
// distinction exists in the entire geometry generator: every tile/rim/
// floor/peg/wall builder below is written purely in terms of (s, t, h)
// and a frame, with no face-specific branching anywhere else.
//
// WINDING-ORDER DERIVATION (done once, used everywhere):
// For a "top-facing" flat quad living at fixed height h, parametrized
// by (s, t), the partial derivatives of P are dP/ds = u, dP/dt = v, so
// a quad whose corners are ordered (s0,t0) -> (s1,t0) -> (s1,t1) ->
// (s0,t1) (i.e. increasing s then increasing t) has triangle normal
//     cross(p1-p0, p2-p0) = (s1-s0)(t1-t0) * cross(u, v)
// which is a positive multiple of cross(u, v) whenever s1>s0, t1>t0.
// So this ordering is correctly outward-facing for ALL SIX faces if
// and only if cross(u, v) == n for all six -- which is exactly what
// verifyFaceFrames() checks below, algebraically, per face. It is NOT
// assumed from symmetry.
//
// For the four vertical "side wall" quads of a footprint spanning
// s in [s0,s1], t in [t0,t1], height h in [h0,h1], the same kind of
// derivation (using the fact that u, v, n form a right-handed
// orthonormal basis with n = u x v, hence v x n = u and n x u = v)
// gives these four winding orders (verified in the comment above each
// wall in addSideWalls below):
//   +s wall (s=s1): (t0,h0)->(t1,h0)->(t1,h1)->(t0,h1)  => normal +u
//   -s wall (s=s0): (t0,h0)->(t0,h1)->(t1,h1)->(t1,h0)  => normal -u
//   +t wall (t=t1): (s0,h0)->(s0,h1)->(s1,h1)->(s1,h0)  => normal +v
//   -t wall (t=t0): (s0,h0)->(s1,h0)->(s1,h1)->(s0,h1)  => normal -v
// These four rules are derived once, generically, from the abstract
// right-handed (u,v,n) relationship, so they too apply to all six
// faces with no per-face special-casing.

struct FaceFrame {
    Vec3 n; // outward unit normal
    Vec3 u; // in-plane axis for local "s" coordinate
    Vec3 v; // in-plane axis for local "t" coordinate
};

// One frame per FaceId, in FaceId enum order (PosZ, NegZ, PosX, NegX, PosY, NegY).
const std::array<FaceFrame, kFaceCount>& faceFrames() {
    static const std::array<FaceFrame, kFaceCount> frames = {{
        /*PosZ*/ {Vec3{0, 0, 1}, Vec3{1, 0, 0}, Vec3{0, 1, 0}},
        /*NegZ*/ {Vec3{0, 0, -1}, Vec3{-1, 0, 0}, Vec3{0, 1, 0}},
        /*PosX*/ {Vec3{1, 0, 0}, Vec3{0, 0, -1}, Vec3{0, 1, 0}},
        /*NegX*/ {Vec3{-1, 0, 0}, Vec3{0, 0, 1}, Vec3{0, 1, 0}},
        /*PosY*/ {Vec3{0, 1, 0}, Vec3{1, 0, 0}, Vec3{0, 0, -1}},
        /*NegY*/ {Vec3{0, -1, 0}, Vec3{1, 0, 0}, Vec3{0, 0, 1}},
    }};
    return frames;
}

Vec3 toWorld(const FaceFrame& f, double half, double s, double t, double h) {
    return f.n * (half + h) + f.u * s + f.v * t;
}

// Adds a quad and, as a verification step, asserts that the resulting
// triangle normal genuinely points along `outwardHint` (dot product
// strictly positive). This means every single quad added anywhere in
// this file is checked at generation time -- not just sampled after
// the fact -- so a winding mistake on any one of the six faces would
// fail loudly (assert) instead of silently producing inward geometry.
void addOutwardQuad(Mesh& mesh, const Vec3& p0, const Vec3& p1, const Vec3& p2,
                     const Vec3& p3, const Vec3& outwardHint) {
    const Vec3 normal = cross(p1 - p0, p2 - p0);
    assert(dot(normal, outwardHint) > 1e-12 &&
           "Quad winding does not point along the expected outward direction");
    (void)outwardHint; // silence unused-variable warning in NDEBUG builds
    mesh.addQuad(p0, p1, p2, p3);
}

// A single flat, top-facing quad at height h covering [s0,s1] x [t0,t1].
void addFlatQuad(Mesh& mesh, const FaceFrame& f, double half, double s0, double s1,
                  double t0, double t1, double h) {
    const Vec3 p0 = toWorld(f, half, s0, t0, h);
    const Vec3 p1 = toWorld(f, half, s1, t0, h);
    const Vec3 p2 = toWorld(f, half, s1, t1, h);
    const Vec3 p3 = toWorld(f, half, s0, t1, h);
    addOutwardQuad(mesh, p0, p1, p2, p3, f.n);
}

// A flat, top-facing "picture frame" at height h: the region inside
// [outerS0,outerS1] x [outerT0,outerT1] minus the inner rectangular hole
// [innerS0,innerS1] x [innerT0,innerT1] (inner strictly inside outer).
// Built from four non-overlapping flat quads (bottom/top/left/right
// strips) so no polygon-with-a-hole machinery is ever needed.
void addFlatFrame(Mesh& mesh, const FaceFrame& f, double half, double outerS0,
                   double outerS1, double outerT0, double outerT1, double innerS0,
                   double innerS1, double innerT0, double innerT1, double h) {
    // bottom strip: full outer width, from outer bottom up to inner bottom
    addFlatQuad(mesh, f, half, outerS0, outerS1, outerT0, innerT0, h);
    // top strip: full outer width, from inner top up to outer top
    addFlatQuad(mesh, f, half, outerS0, outerS1, innerT1, outerT1, h);
    // left strip: between bottom/top strips, from outer left to inner left
    addFlatQuad(mesh, f, half, outerS0, innerS0, innerT0, innerT1, h);
    // right strip: between bottom/top strips, from inner right to outer right
    addFlatQuad(mesh, f, half, innerS1, outerS1, innerT0, innerT1, h);
}

// The four vertical walls connecting footprint [s0,s1] x [t0,t1] at
// height h0 up to the same footprint at height h1. Winding orders are
// exactly as derived in the comment block above.
void addSideWalls(Mesh& mesh, const FaceFrame& f, double half, double s0, double s1,
                   double t0, double t1, double h0, double h1) {
    // +s wall (s = s1): order (t0,h0)->(t1,h0)->(t1,h1)->(t0,h1) => normal +u
    addOutwardQuad(mesh,
                    toWorld(f, half, s1, t0, h0), toWorld(f, half, s1, t1, h0),
                    toWorld(f, half, s1, t1, h1), toWorld(f, half, s1, t0, h1), f.u);
    // -s wall (s = s0): order (t0,h0)->(t0,h1)->(t1,h1)->(t1,h0) => normal -u
    addOutwardQuad(mesh,
                    toWorld(f, half, s0, t0, h0), toWorld(f, half, s0, t0, h1),
                    toWorld(f, half, s0, t1, h1), toWorld(f, half, s0, t1, h0), f.u * -1.0);
    // +t wall (t = t1): order (s0,h0)->(s0,h1)->(s1,h1)->(s1,h0) => normal +v
    addOutwardQuad(mesh,
                    toWorld(f, half, s0, t1, h0), toWorld(f, half, s0, t1, h1),
                    toWorld(f, half, s1, t1, h1), toWorld(f, half, s1, t1, h0), f.v);
    // -t wall (t = t0): order (s0,h0)->(s1,h0)->(s1,h1)->(s0,h1) => normal -v
    addOutwardQuad(mesh,
                    toWorld(f, half, s0, t0, h0), toWorld(f, half, s1, t0, h0),
                    toWorld(f, half, s1, t0, h1), toWorld(f, half, s0, t0, h1), f.v * -1.0);
}

// Builds one face's full 3x3-tiles-of-4x4-pegs structure into `mesh`,
// reading peg on/off state from `grid`. This function is the single
// generic implementation shared by all six faces -- it only ever reads
// `frame` and `grid`, never branches on which face it is.
void buildFace(Mesh& mesh, const FaceFrame& frame, double half, const FaceGrid& grid,
               const GeometryParams& p) {
    const double cellWidth = p.cubeSize / kTileCount;
    const double gapAbs = cellWidth * p.tileGapRatio;
    const double tileFootprintWidth = cellWidth - gapAbs;
    const double rimAbs = tileFootprintWidth * p.tileRimRatio;
    const double innerFloorWidth = tileFootprintWidth - 2.0 * rimAbs;
    const double pegCellWidth = innerFloorWidth / kPegsPerTileSide;
    const double pegMarginAbs = pegCellWidth * p.pegMarginRatio;

    for (int tileRow = 0; tileRow < kTileCount; ++tileRow) {
        for (int tileCol = 0; tileCol < kTileCount; ++tileCol) {
            const double cellS0 = -half + tileCol * cellWidth;
            const double cellS1 = cellS0 + cellWidth;
            const double cellT0 = -half + tileRow * cellWidth;
            const double cellT1 = cellT0 + cellWidth;

            const double tileS0 = cellS0 + gapAbs * 0.5;
            const double tileS1 = cellS1 - gapAbs * 0.5;
            const double tileT0 = cellT0 + gapAbs * 0.5;
            const double tileT1 = cellT1 - gapAbs * 0.5;

            // Level 0 -> base panel: the gap ring around this tile, flat
            // at height 0 (the cube's nominal surface).
            addFlatFrame(mesh, frame, half, cellS0, cellS1, cellT0, cellT1, tileS0,
                         tileS1, tileT0, tileT1, 0.0);

            // Walls connecting the base panel up to the tile top.
            addSideWalls(mesh, frame, half, tileS0, tileS1, tileT0, tileT1, 0.0,
                         p.tileRaiseHeight);

            // Level 1 -> tile top: rim frame + inner floor, both flat at
            // tileRaiseHeight (the rim/floor distinction is purely about
            // which part hosts pegs, not a height difference).
            const double floorS0 = tileS0 + rimAbs;
            const double floorS1 = tileS1 - rimAbs;
            const double floorT0 = tileT0 + rimAbs;
            const double floorT1 = tileT1 - rimAbs;
            addFlatFrame(mesh, frame, half, tileS0, tileS1, tileT0, tileT1, floorS0,
                         floorS1, floorT0, floorT1, p.tileRaiseHeight);

            // Inner floor subdivided into the 4x4 peg sub-grid.
            for (int pegRow = 0; pegRow < kPegsPerTileSide; ++pegRow) {
                for (int pegCol = 0; pegCol < kPegsPerTileSide; ++pegCol) {
                    const double cellPS0 = floorS0 + pegCol * pegCellWidth;
                    const double cellPS1 = cellPS0 + pegCellWidth;
                    const double cellPT0 = floorT0 + pegRow * pegCellWidth;
                    const double cellPT1 = cellPT0 + pegCellWidth;

                    if (!grid.pegAt(tileRow, tileCol, pegRow, pegCol)) {
                        // Off bit: this cell is simply more flat inner floor.
                        addFlatQuad(mesh, frame, half, cellPS0, cellPS1, cellPT0,
                                    cellPT1, p.tileRaiseHeight);
                        continue;
                    }

                    // On bit: margin (flat floor) framing the peg, walls up
                    // to the peg cap, and the cap itself (Level 2).
                    const double pegS0 = cellPS0 + pegMarginAbs;
                    const double pegS1 = cellPS1 - pegMarginAbs;
                    const double pegT0 = cellPT0 + pegMarginAbs;
                    const double pegT1 = cellPT1 - pegMarginAbs;

                    addFlatFrame(mesh, frame, half, cellPS0, cellPS1, cellPT0,
                                 cellPT1, pegS0, pegS1, pegT0, pegT1, p.tileRaiseHeight);
                    addSideWalls(mesh, frame, half, pegS0, pegS1, pegT0, pegT1,
                                 p.tileRaiseHeight, p.tileRaiseHeight + p.pegHeight);
                    addFlatQuad(mesh, frame, half, pegS0, pegS1, pegT0, pegT1,
                                p.tileRaiseHeight + p.pegHeight);
                }
            }
        }
    }
}

} // namespace

bool verifyFaceFrames() {
    bool allOk = true;
    for (const FaceFrame& f : faceFrames()) {
        const Vec3 computed = cross(f.u, f.v);
        const Vec3 diff = computed - f.n;
        const bool ok = length(diff) < 1e-9;
        allOk = allOk && ok;
    }
    return allOk;
}

Mesh generateCubeMesh(const CubeData& data, const GeometryParams& params) {
    Mesh mesh;
    const double half = params.cubeSize / 2.0;
    const auto& frames = faceFrames();
    for (int i = 0; i < kFaceCount; ++i) {
        const FaceId id = static_cast<FaceId>(i);
        buildFace(mesh, frames[static_cast<size_t>(i)], half, data.face(id), params);
    }
    return mesh;
}

} // namespace cube3d
