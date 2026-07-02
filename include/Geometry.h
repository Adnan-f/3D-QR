#pragma once
// Turns bit data (CubeData) + parameters into a Mesh. Has no knowledge
// of file I/O (that's ObjExporter's job) and no knowledge of where the
// bit data came from (that's main()'s/a future decoder's job).

#include "BitGrid.h"
#include "Mesh.h"

namespace cube3d {

struct GeometryParams {
    double cubeSize = 2.0;          // full side length of the base cube
    double tileRaiseHeight = 0.05;  // tile top height above base panel
    double pegHeight = 0.035;       // peg cap height above tile inner floor
    double tileGapRatio = 0.10;     // gap between tiles, as a fraction of one cell's width
    double tileRimRatio = 0.18;     // rim thickness, as a fraction of tile footprint width
    double pegMarginRatio = 0.20;   // peg margin, as a fraction of one peg cell's width
};

// Builds the complete six-face mesh from the given bit data and params.
Mesh generateCubeMesh(const CubeData& data, const GeometryParams& params);

// Algebraically verifies that every one of the six face frames used by
// generateCubeMesh satisfies cross(u, v) == n (i.e. produces correctly
// outward-facing normals for the "top-facing" winding rule). Returns
// true iff all six pass. Intended to be called once at program startup,
// independent of any actual mesh generation, as an explicit derivation
// check rather than an assumption based on symmetry.
bool verifyFaceFrames();

} // namespace cube3d
