// main.cpp -- demo entry point.
// explanation tsotsotra for the data
// To plug in different data, edit populateDemoPatterns() below: it is
// the ONLY place hardcoded bit patterns live. A future automated
// decoder would simply call CubeData::face(id).set(row,col,value) (or
// .setPeg(...)) the same way, with no changes needed anywhere else in
// this project.

#include "BitGrid.h"
#include "Geometry.h"
#include "ObjExporter.h"

#include <cstdio>
#include <cmath>
#include <string>

using namespace cube3d;

namespace {

void populateDemoPatterns(CubeData& data) {
    // PosZ: all pegs on, EXCEPT the center tile (tile row 1, col 1) is
    // forced fully blank -- this is the required demonstration of the
    // "blank tile" case (rim + recessed floor, zero pegs), shown
    // alongside tiles that do have pegs.
    {
        FaceGrid& g = data.face(FaceId::PosZ);
        for (int r = 0; r < kGridResolution; ++r)
            for (int c = 0; c < kGridResolution; ++c) g.set(r, c, true);
        for (int r = 4; r < 8; ++r)
            for (int c = 4; c < 8; ++c) g.set(r, c, false);
    }
    // NegZ: checkerboard.
    {
        FaceGrid& g = data.face(FaceId::NegZ);
        for (int r = 0; r < kGridResolution; ++r)
            for (int c = 0; c < kGridResolution; ++c) g.set(r, c, (r + c) % 2 == 0);
    }
    // PosX: horizontal stripes.
    {
        FaceGrid& g = data.face(FaceId::PosX);
        for (int r = 0; r < kGridResolution; ++r)
            for (int c = 0; c < kGridResolution; ++c) g.set(r, c, r % 2 == 0);
    }
    // NegX: vertical stripes.
    {
        FaceGrid& g = data.face(FaceId::NegX);
        for (int r = 0; r < kGridResolution; ++r)
            for (int c = 0; c < kGridResolution; ++c) g.set(r, c, c % 2 == 0);
    }
    // PosY: diagonal bands.
    {
        FaceGrid& g = data.face(FaceId::PosY);
        for (int r = 0; r < kGridResolution; ++r)
            for (int c = 0; c < kGridResolution; ++c) g.set(r, c, (r + c) % 3 == 0);
    }
    // NegY: a hollow ring per-tile (only the outer edge pegs of each
    // 4x4 tile sub-grid are on).
    {
        FaceGrid& g = data.face(FaceId::NegY);
        for (int tr = 0; tr < kTileCount; ++tr) {
            for (int tc = 0; tc < kTileCount; ++tc) {
                for (int pr = 0; pr < kPegsPerTileSide; ++pr) {
                    for (int pc = 0; pc < kPegsPerTileSide; ++pc) {
                        const bool onEdge =
                            pr == 0 || pr == kPegsPerTileSide - 1 || pc == 0 ||
                            pc == kPegsPerTileSide - 1;
                        g.setPeg(tr, tc, pr, pc, onEdge);
                    }
                }
            }
        }
    }
}

// Independently computes the expected vertex/triangle counts from the
// bit data + the known geometric decomposition, WITHOUT touching the
// Mesh produced by generateCubeMesh. Used as a cross-check in main().
struct ExpectedCounts {
    long long vertices = 0;
    long long triangles = 0;
};

ExpectedCounts computeExpectedCounts(const CubeData& data) {
    // Per tile, fixed structure (independent of peg pattern):
    //   base-panel gap frame : 4 quads -> 16 verts, 8 tris
    //   side walls           : 4 quads -> 16 verts, 8 tris
    //   rim frame            : 4 quads -> 16 verts, 8 tris
    // => 48 verts, 24 tris fixed per tile.
    // Per peg cell (16 per tile):
    //   off -> 1 quad  ->  4 verts,  2 tris
    //   on  -> margin frame (4 quads=16v,8t) + walls (4 quads=16v,8t)
    //          + cap (1 quad=4v,2t) -> 36 verts, 18 tris
    constexpr long long kFixedVertsPerTile = 48;
    constexpr long long kFixedTrisPerTile = 24;
    constexpr long long kOffVerts = 4, kOffTris = 2;
    constexpr long long kOnVerts = 36, kOnTris = 18;

    ExpectedCounts result;
    for (int fi = 0; fi < kFaceCount; ++fi) {
        const FaceGrid& g = data.face(static_cast<FaceId>(fi));
        for (int tr = 0; tr < kTileCount; ++tr) {
            for (int tc = 0; tc < kTileCount; ++tc) {
                result.vertices += kFixedVertsPerTile;
                result.triangles += kFixedTrisPerTile;
                for (int pr = 0; pr < kPegsPerTileSide; ++pr) {
                    for (int pc = 0; pc < kPegsPerTileSide; ++pc) {
                        if (g.pegAt(tr, tc, pr, pc)) {
                            result.vertices += kOnVerts;
                            result.triangles += kOnTris;
                        } else {
                            result.vertices += kOffVerts;
                            result.triangles += kOffTris;
                        }
                    }
                }
            }
        }
    }
    return result;
}

// Signed volume of a closed triangle mesh via the divergence theorem:
// V = (1/6) * sum_over_triangles( dot(a, cross(b, c)) ).
double signedVolume(const Mesh& mesh) {
    double total = 0.0;
    const auto& verts = mesh.vertices();
    for (const Triangle& t : mesh.triangles()) {
        const Vec3& a = verts[static_cast<size_t>(t.a)];
        const Vec3& b = verts[static_cast<size_t>(t.b)];
        const Vec3& c = verts[static_cast<size_t>(t.c)];
        total += dot(a, cross(b, c));
    }
    return total / 6.0;
}

// Independently-derived expected solid volume: base cube + raised tile
// blocks (9 per face x 6 faces) + raised peg blocks (one per set bit).
double computeExpectedVolume(const CubeData& data, const GeometryParams& p) {
    const double cellWidth = p.cubeSize / kTileCount;
    const double gapAbs = cellWidth * p.tileGapRatio;
    const double tileFootprintWidth = cellWidth - gapAbs;
    const double rimAbs = tileFootprintWidth * p.tileRimRatio;
    const double innerFloorWidth = tileFootprintWidth - 2.0 * rimAbs;
    const double pegCellWidth = innerFloorWidth / kPegsPerTileSide;
    const double pegMarginAbs = pegCellWidth * p.pegMarginRatio;
    const double pegFootprintWidth = pegCellWidth - 2.0 * pegMarginAbs;

    const double baseCubeVolume = p.cubeSize * p.cubeSize * p.cubeSize;
    const double tileVolumeEach = tileFootprintWidth * tileFootprintWidth * p.tileRaiseHeight;
    const double pegVolumeEach = pegFootprintWidth * pegFootprintWidth * p.pegHeight;

    const long long tileCountTotal =
        static_cast<long long>(kFaceCount) * kTileCount * kTileCount;
    const long long onPegCount = data.totalSetBits();

    return baseCubeVolume + static_cast<double>(tileCountTotal) * tileVolumeEach +
           static_cast<double>(onPegCount) * pegVolumeEach;
}

} // namespace

int main() {
    // 1) Build the binary data model and fill it with demo patterns.
    CubeData data;
    populateDemoPatterns(data);

    // 2) Verify the six face frames algebraically before generating
    //    anything from them.
    const bool framesOk = verifyFaceFrames();
    std::printf("Face-frame winding verification (cross(u,v) == n, all six faces): %s\n",
                framesOk ? "PASS" : "FAIL");
    if (!framesOk) {
        std::fprintf(stderr, "Aborting: face frames are not consistent with outward winding.\n");
        return 1;
    }

    // 3) Generate the mesh. Every quad added during generation is
    //    itself checked via assert() against its expected outward
    //    direction (see addOutwardQuad in Geometry.cpp) -- reaching
    //    this point with no assertion failure already proves every
    //    single quad's normal matched its intended outward direction.
    GeometryParams params; // defaults
    const Mesh mesh = generateCubeMesh(data, params);

    // 4) Export.
    const std::string outPath = "cube.obj";
    const bool exported = exportObj(mesh, outPath);
    std::printf("Export to %s: %s\n", outPath.c_str(), exported ? "OK" : "FAILED");
    if (!exported) return 1;

    // 5) Stats.
    std::printf("\n--- Mesh statistics ---\n");
    std::printf("Total vertices : %zu\n", mesh.vertices().size());
    std::printf("Total triangles: %zu\n", mesh.triangles().size());
    std::printf("Total bits set : %d / %d\n", data.totalSetBits(), kTotalBits);
    static const char* kFaceNames[kFaceCount] = {"PosZ", "NegZ", "PosX", "NegX", "PosY", "NegY"};
    for (int i = 0; i < kFaceCount; ++i) {
        std::printf("  Face %s: %d / %d bits set\n", kFaceNames[i],
                    data.face(static_cast<FaceId>(i)).countSetBits(), kBitsPerFace);
    }

    // 6) Independent vertex/triangle count cross-check.
    const ExpectedCounts expected = computeExpectedCounts(data);
    const bool countsOk = (static_cast<long long>(mesh.vertices().size()) == expected.vertices) &&
                           (static_cast<long long>(mesh.triangles().size()) == expected.triangles);
    std::printf("\n--- Verification ---\n");
    std::printf("Expected vertices (independent formula) : %lld (actual %zu)\n",
                expected.vertices, mesh.vertices().size());
    std::printf("Expected triangles (independent formula): %lld (actual %zu)\n",
                expected.triangles, mesh.triangles().size());
    std::printf("Vertex/triangle count cross-check: %s\n", countsOk ? "PASS" : "FAIL");

    // 7) Divergence-theorem signed volume check.
    const double vol = signedVolume(mesh);
    const double expectedVol = computeExpectedVolume(data, params);
    const double relErr = std::fabs(vol - expectedVol) / expectedVol;
    std::printf("Signed volume (divergence theorem) : %.6f\n", vol);
    std::printf("Expected volume (independent formula): %.6f\n", expectedVol);
    std::printf("Relative error: %.8f\n", relErr);
    const bool volumeOk = vol > 0.0 && relErr < 1e-6;
    std::printf("Volume check (positive & matches expected): %s\n", volumeOk ? "PASS" : "FAIL");

    const bool allOk = framesOk && countsOk && volumeOk;
    std::printf("\nOVERALL: %s\n", allOk ? "ALL CHECKS PASSED" : "SOME CHECKS FAILED");
    return allOk ? 0 : 1;
}
