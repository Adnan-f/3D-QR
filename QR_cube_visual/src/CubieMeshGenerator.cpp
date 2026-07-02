// CubieMeshGenerator.cpp
//
// See CubieMeshGenerator.hpp for the high-level construction summary.
// This file contains the actual triangle generation plus the derivation
// and verification of the six face frames.

#include "CubieMeshGenerator.hpp"

#include <cassert>
#include <cmath>
#include <iostream>

namespace qrcube {

namespace {

// ---------------------------------------------------------------------------
// addQuad: adds a planar quad (two triangles) given four corners in a
// consistent order, and a *desired* outward normal. We compute the normal
// implied by the corner winding (p0,p1,p2,p3 assumed to go around the quad
// in order, either CW or CCW) and, if it disagrees with the desired
// normal, swap the winding so the final triangles always wind correctly
// for the desired normal. This makes every call site self-correcting: even
// if the caller's corner order is wrong for some face, the emitted
// triangles still face the right way -- this directly addresses the
// "verify winding produces outward-facing normals on ALL six frames" sharp
// edge called out in the spec, by making correctness structural rather
// than relying on getting U/V order right by hand for every feature.
// ---------------------------------------------------------------------------
void addQuad(std::vector<Vertex>& verts, std::vector<uint32_t>& idx,
             glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, glm::vec3 p3,
             glm::vec3 desiredNormal) {
    glm::vec3 impliedNormal = glm::cross(p1 - p0, p2 - p0);
    // Degenerate (near-zero area) quads can have a near-zero implied
    // normal; fall back to the desired normal for the dot-product check in
    // that case so we don't flip based on noise.
    float lenSq = glm::dot(impliedNormal, impliedNormal);
    bool needsFlip = false;
    if (lenSq > 1e-12f) {
        needsFlip = glm::dot(impliedNormal, desiredNormal) < 0.0f;
    }
    if (needsFlip) {
        std::swap(p1, p3);
    }

    glm::vec3 n = glm::normalize(desiredNormal);
    uint32_t base = static_cast<uint32_t>(verts.size());
    verts.push_back({p0, n});
    verts.push_back({p1, n});
    verts.push_back({p2, n});
    verts.push_back({p3, n});

    idx.push_back(base + 0);
    idx.push_back(base + 1);
    idx.push_back(base + 2);
    idx.push_back(base + 0);
    idx.push_back(base + 2);
    idx.push_back(base + 3);
}

// Convenience: map a local (u, v, h) coordinate on a face frame to a 3D
// position in cubie-local space. This is the single formula reused for
// every feature on every face, as documented in the header.
inline glm::vec3 framePoint(const FaceFrame& f, float u, float v, float h) {
    return f.origin + u * f.uAxis + v * f.vAxis + h * f.normal;
}

// Adds a vertical wall (a rectangle whose 4 corners are formed by sweeping
// the segment from local point A=(uA,vA) to B=(uB,vB) between heights
// hLow and hHigh). The wall's outward normal is supplied explicitly by the
// caller (it is perpendicular to the A->B direction, not the face normal).
void addWall(std::vector<Vertex>& verts, std::vector<uint32_t>& idx,
             const FaceFrame& f,
             float uA, float vA, float uB, float vB,
             float hLow, float hHigh,
             glm::vec3 wallNormal) {
    glm::vec3 p0 = framePoint(f, uA, vA, hLow);
    glm::vec3 p1 = framePoint(f, uB, vB, hLow);
    glm::vec3 p2 = framePoint(f, uB, vB, hHigh);
    glm::vec3 p3 = framePoint(f, uA, vA, hHigh);
    addQuad(verts, idx, p0, p1, p2, p3, wallNormal);
}

// Adds a flat horizontal (in u,v) rectangle at height h, with the face's
// own outward normal.
void addFlatRect(std::vector<Vertex>& verts, std::vector<uint32_t>& idx,
                  const FaceFrame& f,
                  float uMin, float uMax, float vMin, float vMax, float h) {
    glm::vec3 p0 = framePoint(f, uMin, vMin, h);
    glm::vec3 p1 = framePoint(f, uMax, vMin, h);
    glm::vec3 p2 = framePoint(f, uMax, vMax, h);
    glm::vec3 p3 = framePoint(f, uMin, vMax, h);
    addQuad(verts, idx, p0, p1, p2, p3, f.normal);
}

// Adds a flat "frame" (rectangle with a rectangular hole in the middle) at
// height h, built from 4 quads (top/bottom/left/right strips). Used for a
// tile's rim top surface (outer footprint minus inner footprint).
void addFlatFrame(std::vector<Vertex>& verts, std::vector<uint32_t>& idx,
                   const FaceFrame& f,
                   float outerUMin, float outerUMax, float outerVMin, float outerVMax,
                   float innerUMin, float innerUMax, float innerVMin, float innerVMax,
                   float h) {
    // Bottom strip (full outer width, from outerVMin to innerVMin)
    addFlatRect(verts, idx, f, outerUMin, outerUMax, outerVMin, innerVMin, h);
    // Top strip (full outer width, from innerVMax to outerVMax)
    addFlatRect(verts, idx, f, outerUMin, outerUMax, innerVMax, outerVMax, h);
    // Left strip (inner height range only, from outerUMin to innerUMin)
    addFlatRect(verts, idx, f, outerUMin, innerUMin, innerVMin, innerVMax, h);
    // Right strip (inner height range only, from innerUMax to outerUMax)
    addFlatRect(verts, idx, f, innerUMax, outerUMax, innerVMin, innerVMax, h);
}

// Adds the four vertical walls of an axis-aligned rectangular footprint,
// from height hLow to hHigh, with outward-pointing normals derived from
// the face's own U/V axes (each wall's normal is +-U or +-V).
void addBoxWalls(std::vector<Vertex>& verts, std::vector<uint32_t>& idx,
                  const FaceFrame& f,
                  float uMin, float uMax, float vMin, float vMax,
                  float hLow, float hHigh) {
    // -V wall (facing -V direction), spans u in [uMin,uMax] at v = vMin
    addWall(verts, idx, f, uMin, vMin, uMax, vMin, hLow, hHigh, -f.vAxis);
    // +V wall, at v = vMax (reverse u order so the quad winds correctly
    // before addQuad's self-correction even kicks in -- addQuad will fix
    // it regardless, but keeping a sensible order avoids relying solely on
    // the correction path).
    addWall(verts, idx, f, uMax, vMax, uMin, vMax, hLow, hHigh, f.vAxis);
    // -U wall, at u = uMin
    addWall(verts, idx, f, uMin, vMax, uMin, vMin, hLow, hHigh, -f.uAxis);
    // +U wall, at u = uMax
    addWall(verts, idx, f, uMax, vMin, uMax, vMax, hLow, hHigh, f.uAxis);
}

// Generates one full tile (rim walls, rim top frame, inner floor walls,
// inner floor, and up to 16 pegs) centered at (centerU, centerV) on face f.
void addTile(std::vector<Vertex>& verts, std::vector<uint32_t>& idx,
             const FaceFrame& f, const CubieMeshParams& p,
             const CubeFaceData& data, int tileRow, int tileCol,
             float centerU, float centerV, float tileFootprint) {
    const float outerHalf = tileFootprint * 0.5f;
    const float innerHalf = outerHalf - p.rimWidth;
    const float tileFloorHeight = p.tileRaiseHeight - p.floorRecess;

    const float outerUMin = centerU - outerHalf, outerUMax = centerU + outerHalf;
    const float outerVMin = centerV - outerHalf, outerVMax = centerV + outerHalf;
    const float innerUMin = centerU - innerHalf, innerUMax = centerU + innerHalf;
    const float innerVMin = centerV - innerHalf, innerVMax = centerV + innerHalf;

    // 1) Outer side walls: base panel (h=0) up to rim top (h=tileRaiseHeight)
    addBoxWalls(verts, idx, f, outerUMin, outerUMax, outerVMin, outerVMax,
                0.0f, p.tileRaiseHeight);

    // 2) Rim top surface: flat frame at h=tileRaiseHeight between outer and
    //    inner footprints.
    addFlatFrame(verts, idx, f, outerUMin, outerUMax, outerVMin, outerVMax,
                 innerUMin, innerUMax, innerVMin, innerVMax, p.tileRaiseHeight);

    // 3) Inner floor side walls: from rim top (h=tileRaiseHeight) down to
    //    the recessed floor (h=tileFloorHeight), around the inner footprint.
    addBoxWalls(verts, idx, f, innerUMin, innerUMax, innerVMin, innerVMax,
                tileFloorHeight, p.tileRaiseHeight);

    // 4) Inner floor surface itself.
    addFlatRect(verts, idx, f, innerUMin, innerUMax, innerVMin, innerVMax, tileFloorHeight);

    // 5) Pegs: 4x4 grid within the inner floor footprint.
    const float innerSize = innerHalf * 2.0f;
    const float pegPitch = innerSize / static_cast<float>(kPegsPerTileSide);
    const float pegSize = pegPitch - p.pegGap;
    const float pegHalf = pegSize * 0.5f;
    const float pegTopHeight = tileFloorHeight + p.pegHeight;

    for (int pr = 0; pr < kPegsPerTileSide; ++pr) {
        for (int pc = 0; pc < kPegsPerTileSide; ++pc) {
            if (!data.pegAt(tileRow, tileCol, pr, pc)) continue;

            // Peg grid coordinates: pr indexes along V, pc indexes along U
            // (row -> V/vertical-in-2D, col -> U/horizontal-in-2D), matching
            // the (row, col) convention used by CubeFaceData.
            const float pegCenterU = innerUMin + pegPitch * (static_cast<float>(pc) + 0.5f);
            const float pegCenterV = innerVMin + pegPitch * (static_cast<float>(pr) + 0.5f);

            const float puMin = pegCenterU - pegHalf, puMax = pegCenterU + pegHalf;
            const float pvMin = pegCenterV - pegHalf, pvMax = pegCenterV + pegHalf;

            // Side walls from the tile floor up to the peg cap.
            addBoxWalls(verts, idx, f, puMin, puMax, pvMin, pvMax,
                        tileFloorHeight, pegTopHeight);
            // Flat top cap.
            addFlatRect(verts, idx, f, puMin, puMax, pvMin, pvMax, pegTopHeight);
        }
    }
}

// Generates the full tiled-peg pattern for one visible face: the base
// panel plus the 3x3 array of tiles.
void addPatternedFace(std::vector<Vertex>& verts, std::vector<uint32_t>& idx,
                       const FaceFrame& f, const CubieMeshParams& p,
                       const CubeFaceData& data) {
    const float half = p.cubieSize * 0.5f;
    const float arrayHalf = half - p.faceMargin;
    const float arraySize = arrayHalf * 2.0f;

    // Base panel covers the entire face footprint (visible in the gaps
    // between/around tiles).
    addFlatRect(verts, idx, f, -half, half, -half, half, 0.0f);

    const float tilePitch = arraySize / static_cast<float>(kTilesPerSide);
    const float tileFootprint = tilePitch - p.tileGap;

    for (int tr = 0; tr < kTilesPerSide; ++tr) {
        for (int tc = 0; tc < kTilesPerSide; ++tc) {
            const float centerU = -arrayHalf + tilePitch * (static_cast<float>(tc) + 0.5f);
            const float centerV = -arrayHalf + tilePitch * (static_cast<float>(tr) + 0.5f);
            addTile(verts, idx, f, p, data, tr, tc, centerU, centerV, tileFootprint);
        }
    }
}

// Generates a plain flat quad for a non-visible (interior) face -- cheap
// placeholder geometry, never seen from outside the assembled cube.
void addPlainFace(std::vector<Vertex>& verts, std::vector<uint32_t>& idx,
                   const FaceFrame& f, float cubieSize) {
    const float half = cubieSize * 0.5f;
    addFlatRect(verts, idx, f, -half, half, -half, half, 0.0f);
}

} // namespace

// ---------------------------------------------------------------------------
// Face frame derivation & verification.
//
// We need, for each of the 6 local faces, an origin O (face center), two
// in-plane axes U,V, and outward normal N such that cross(U,V) == N (so a
// CCW winding in (u,v) -- as produced by addFlatRect's corner order --
// yields a normal matching N, consistent with addQuad's self-correcting
// check). Below each frame's cross product is computed by hand in the
// comment and matches the listed normal; addQuad's runtime check is a
// second, independent safety net on top of this.
//
//   +X: N=( 1,0,0)  U=(0,0,-1)  V=(0,1,0)
//       cross(U,V) = (0*0-(-1)*1, (-1)*0-0*0, 0*1-0*0) = (1,0,0) == N  OK
//
//   -X: N=(-1,0,0)  U=(0,0, 1)  V=(0,1,0)
//       cross(U,V) = (0*0-1*1, 1*0-0*0, 0*1-0*0) = (-1,0,0) == N      OK
//
//   +Y: N=(0, 1,0)  U=(1,0,0)   V=(0,0,-1)
//       cross(U,V) = (0*-1-0*0, 0*0-1*-1, 1*0-0*0) = (0,1,0) == N     OK
//
//   -Y: N=(0,-1,0)  U=(1,0,0)   V=(0,0, 1)
//       cross(U,V) = (0*1-0*0, 0*0-1*1, 1*0-0*0) = (0,-1,0) == N      OK
//
//   +Z: N=(0,0, 1)  U=(1,0,0)   V=(0,1,0)
//       cross(U,V) = (0*0-0*1, 0*0-1*0, 1*1-0*0) = (0,0,1) == N       OK
//
//   -Z: N=(0,0,-1)  U=(0,1,0)   V=(1,0,0)
//       cross(U,V) = (1*0-0*0, 0*1-0*0, 0*0-1*1) = (0,0,-1) == N      OK
//
// All six independently verified -- no two faces share an accidental
// shortcut, and none rely on "the opposite face is just the negation."
// runtimeVerifyFaceFrames() below re-checks this numerically at startup.
// ---------------------------------------------------------------------------
std::array<FaceFrame, kFaceCount> makeCubieFaceFrames(float cubieSize) {
    const float h = cubieSize * 0.5f;
    std::array<FaceFrame, kFaceCount> frames{};

    frames[static_cast<int>(FaceId::PosX)] =
        FaceFrame{glm::vec3(h, 0, 0), glm::vec3(0, 0, -1), glm::vec3(0, 1, 0), glm::vec3(1, 0, 0)};
    frames[static_cast<int>(FaceId::NegX)] =
        FaceFrame{glm::vec3(-h, 0, 0), glm::vec3(0, 0, 1), glm::vec3(0, 1, 0), glm::vec3(-1, 0, 0)};
    frames[static_cast<int>(FaceId::PosY)] =
        FaceFrame{glm::vec3(0, h, 0), glm::vec3(1, 0, 0), glm::vec3(0, 0, -1), glm::vec3(0, 1, 0)};
    frames[static_cast<int>(FaceId::NegY)] =
        FaceFrame{glm::vec3(0, -h, 0), glm::vec3(1, 0, 0), glm::vec3(0, 0, 1), glm::vec3(0, -1, 0)};
    frames[static_cast<int>(FaceId::PosZ)] =
        FaceFrame{glm::vec3(0, 0, h), glm::vec3(1, 0, 0), glm::vec3(0, 1, 0), glm::vec3(0, 0, 1)};
    frames[static_cast<int>(FaceId::NegZ)] =
        FaceFrame{glm::vec3(0, 0, -h), glm::vec3(0, 1, 0), glm::vec3(1, 0, 0), glm::vec3(0, 0, -1)};

    // Runtime cross-product verification (belt-and-suspenders alongside
    // the hand-derivation in the comment block above).
    for (auto& f : frames) {
        glm::vec3 cross = glm::cross(f.uAxis, f.vAxis);
        float dot = glm::dot(glm::normalize(cross), glm::normalize(f.normal));
        if (dot < 0.999f) {
            std::cerr << "[CubieMeshGenerator] FATAL: face frame winding does not "
                         "match its declared outward normal (dot=" << dot << ")\n";
            assert(false && "face frame winding verification failed");
        }
    }
    return frames;
}

CubieMesh generateCubieMesh(const CubieMeshParams& params,
                             const std::array<bool, kFaceCount>& visibleFaces,
                             const std::array<CubeFaceData, kFaceCount>& faceData) {
    CubieMesh mesh;
    auto frames = makeCubieFaceFrames(params.cubieSize);

    for (int i = 0; i < kFaceCount; ++i) {
        if (visibleFaces[i]) {
            addPatternedFace(mesh.vertices, mesh.indices, frames[i], params, faceData[i]);
        } else {
            addPlainFace(mesh.vertices, mesh.indices, frames[i], params.cubieSize);
        }
    }
    return mesh;
}

} // namespace qrcube
