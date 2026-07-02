// RubiksCubeTest.cpp
//
// Standalone verification harness for the Part 3 requirements:
//   (a) applying any single face turn four times in a row returns every
//       cubie to its original position AND original orientation (identity)
//   (b) a single turn only moves the 9 cubies in the selected layer and
//       leaves the other 18 untouched
//
// This program has NO OpenGL/GLFW/window dependency -- it links only
// RubiksCube.cpp + Cubie.hpp (and a minimal GLM-API-compatible shim, see
// test/glm/, used only because the real GLM library is unavailable for
// offline compilation in this verification environment; the shim
// implements the same function signatures/semantics relied upon by
// RubiksCube.hpp/.cpp so the logic under test is unmodified).
//
// Run it with no arguments; it prints PASS/FAIL for every check and
// returns a non-zero exit code if anything fails.

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#include "RubiksCube.hpp"

using namespace qrcube;

namespace {

int g_failures = 0;

void check(bool condition, const char* description) {
    if (condition) {
        std::printf("  [PASS] %s\n", description);
    } else {
        std::printf("  [FAIL] %s\n", description);
        ++g_failures;
    }
}

std::vector<Cubie> makeSolvedCubies() {
    std::vector<Cubie> cubies;
    cubies.reserve(27);
    for (int x = -1; x <= 1; ++x)
        for (int y = -1; y <= 1; ++y)
            for (int z = -1; z <= 1; ++z) {
                Cubie c(glm::ivec3(x, y, z), MeshRange{});
                cubies.push_back(c);
            }
    return cubies;
}

bool quatsApproxEqual(const glm::quat& a, const glm::quat& b, float eps = 1e-3f) {
    // Quaternions q and -q represent the same rotation.
    float d1 = std::abs(a.w - b.w) + std::abs(a.x - b.x) + std::abs(a.y - b.y) + std::abs(a.z - b.z);
    float d2 = std::abs(a.w + b.w) + std::abs(a.x + b.x) + std::abs(a.y + b.y) + std::abs(a.z + b.z);
    return d1 < eps || d2 < eps;
}

const char* axisName(Axis a) {
    switch (a) {
        case Axis::X: return "X";
        case Axis::Y: return "Y";
        case Axis::Z: return "Z";
    }
    return "?";
}

} // namespace

int main() {
    std::printf("=== RubiksCube move-logic verification ===\n\n");

    const std::vector<std::string> moveNames = {"U", "U'", "D", "D'", "R", "R'",
                                                  "L", "L'", "F", "F'", "B", "B'"};

    // ---- Check (a): each move applied 4x returns to identity ----
    std::printf("-- Check (a): 4x repeated turn returns to identity --\n");
    for (const auto& name : moveNames) {
        auto reqOpt = lookupMove(name);
        check(reqOpt.has_value(), (std::string("lookupMove resolves '") + name + "'").c_str());
        if (!reqOpt.has_value()) continue;
        TurnRequest req = *reqOpt;

        RubiksCube cube(makeSolvedCubies());
        // Snapshot original state indexed by original cubie identity (we
        // use index since RubiksCube's vector of cubies never reorders --
        // only gridPos/orientation of cubies mutate).
        std::vector<glm::ivec3> origPos;
        std::vector<glm::quat> origOri;
        for (int i = 0; i < cube.cubieCount(); ++i) {
            origPos.push_back(cube.cubieAt(i).gridPos());
            origOri.push_back(cube.cubieAt(i).orientation());
        }

        for (int t = 0; t < 4; ++t) {
            cube.applyTurnImmediate(req);
        }

        bool allMatch = true;
        for (int i = 0; i < cube.cubieCount(); ++i) {
            if (cube.cubieAt(i).gridPos() != origPos[static_cast<size_t>(i)]) allMatch = false;
            if (!quatsApproxEqual(cube.cubieAt(i).orientation(), origOri[static_cast<size_t>(i)])) allMatch = false;
        }
        check(allMatch, (std::string("move '") + name + "' x4 == identity (position + orientation)").c_str());
    }

    // ---- Check (b): a single turn only moves the 9 layer cubies ----
    std::printf("\n-- Check (b): single turn touches exactly the 9 selected cubies --\n");
    for (const auto& name : moveNames) {
        auto reqOpt = lookupMove(name);
        if (!reqOpt.has_value()) continue;
        TurnRequest req = *reqOpt;

        RubiksCube cube(makeSolvedCubies());
        std::vector<glm::ivec3> origPos;
        std::vector<glm::quat> origOri;
        for (int i = 0; i < cube.cubieCount(); ++i) {
            origPos.push_back(cube.cubieAt(i).gridPos());
            origOri.push_back(cube.cubieAt(i).orientation());
        }

        cube.applyTurnImmediate(req);

        int movedCount = 0, untouchedCorrect = 0;
        for (int i = 0; i < cube.cubieCount(); ++i) {
            bool posChanged = cube.cubieAt(i).gridPos() != origPos[static_cast<size_t>(i)];
            bool oriChanged = !quatsApproxEqual(cube.cubieAt(i).orientation(), origOri[static_cast<size_t>(i)]);
            bool inLayer;
            glm::ivec3 p = origPos[static_cast<size_t>(i)];
            int coord = (req.axis == Axis::X) ? p.x : (req.axis == Axis::Y ? p.y : p.z);
            inLayer = (coord == req.layer);

            if (inLayer) {
                if (posChanged || oriChanged) ++movedCount;
            } else {
                if (!posChanged && !oriChanged) ++untouchedCorrect;
            }
        }
        char buf[160];
        std::snprintf(buf, sizeof(buf),
                      "move '%s' (axis=%s layer=%d): %d/9 layer cubies changed, %d/18 others untouched",
                      name.c_str(), axisName(req.axis), req.layer, movedCount, untouchedCorrect);
        check(movedCount == 9 && untouchedCorrect == 18, buf);
    }

    // ---- Check (c): slot mapping stays a valid bijection after many turns ----
    std::printf("\n-- Check (c): slot mapping remains a valid bijection after a long scramble --\n");
    {
        RubiksCube cube(makeSolvedCubies());
        const std::vector<std::string> scramble = {"U", "R", "F'", "D", "L'", "B", "U'", "R'",
                                                     "F", "D'", "L", "B'", "R", "R", "U", "U'"};
        bool ok = true;
        for (const auto& mv : scramble) {
            auto req = lookupMove(mv);
            cube.applyTurnImmediate(*req);
            // Verify bijection: every one of the 27 slots maps to exactly
            // one cubie, and each cubie's gridPos is unique.
            std::vector<int> seen(27, 0);
            for (int x = -1; x <= 1; ++x)
                for (int y = -1; y <= 1; ++y)
                    for (int z = -1; z <= 1; ++z) {
                        int idx = cube.cubieIndexAtSlot(glm::ivec3(x, y, z));
                        if (idx < 0 || idx >= 27) { ok = false; }
                        else seen[static_cast<size_t>(idx)]++;
                    }
            for (int s : seen) if (s != 1) ok = false;
        }
        check(ok, "after 16-move scramble, every slot maps to exactly one cubie (no duplicates/empties)");
    }

    std::printf("\n=== %s (%d failure%s) ===\n", g_failures == 0 ? "ALL CHECKS PASSED" : "CHECKS FAILED",
                g_failures, g_failures == 1 ? "" : "s");
    return g_failures == 0 ? 0 : 1;
}
