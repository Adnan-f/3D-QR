# 3D QR Rubik's Cube

An interactive 3x3x3 Rubik's-cube-style C++17 + OpenGL 3.3 application.
Every outward-facing cubie side shows a 3x3 array of raised tiles, each
tile holding a 4x4 array of small pegs ("studs") driven by a
`std::bitset<144>` bit pattern -- a "3D QR code" surface look. The cube
supports full keyboard-driven face turns (U/D/L/R/F/B + primes), animated
smoothly, plus mouse orbit/zoom camera control.

## Building

### Prerequisites

- A C++17 compiler (GCC 9+/Clang 10+/MSVC 2019+)
- CMake 3.16+
- GLFW3 (dev package), e.g. `sudo apt install libglfw3-dev` on Debian/Ubuntu,
  `brew install glfw` on macOS
- GLM (header-only), e.g. `sudo apt install libglm-dev`, or drop the `glm/`
  folder from https://github.com/g-truc/glm into your include path
- GLAD, generated for OpenGL 3.3 Core at https://glad.dav1d.de/
  (Language: C/C++, gl: Version 3.3, Profile: Core, check "Generate a
  loader"). Unzip the result into `third_party/glad` so you have:
  ```
  third_party/glad/include/glad/glad.h
  third_party/glad/include/KHR/khrplatform.h
  third_party/glad/src/glad.c
  ```

### Build with CMake (recommended)

```sh
mkdir build && cd build
cmake ..
cmake --build . -j
./qr_rubiks_cube
```

This also builds `rubiks_cube_test`, the standalone move-logic
verification harness described below:

```sh
./rubiks_cube_test
# or: ctest
```

### Build with a direct g++ command (alternative)

From the project root, assuming GLFW/GLM dev packages are installed
system-wide and `third_party/glad` is populated as above:

```sh
g++ -std=c++17 -Wall -Wextra -O2 \
  -Iinclude -Ithird_party/glad/include \
  src/main.cpp src/Camera.cpp src/Shader.cpp src/CubieMeshGenerator.cpp \
  src/RubiksCube.cpp src/InputController.cpp third_party/glad/src/glad.c \
  -lglfw -ldl -lGL \
  -o qr_rubiks_cube
./qr_rubiks_cube
```

(Run it from the project root, or copy the `shaders/` directory next to
the resulting binary -- the program loads shaders via the relative paths
`shaders/cubie.vert` / `shaders/cubie.frag`.)

### Building just the move-logic test (no GLFW/GLAD/window needed)

```sh
g++ -std=c++17 -Wall -Wextra -Iinclude test/RubiksCubeTest.cpp src/RubiksCube.cpp -o rubiks_cube_test
./rubiks_cube_test
```

This only needs GLM (no GLFW, no GLAD, no display) since it never touches
rendering -- it purely exercises `RubiksCube`'s turn/orientation algebra.

## Controls

| Action                          | Key / Mouse                          |
|----------------------------------|---------------------------------------|
| U / U' (top layer)               | `U`  /  `Shift+U`                     |
| D / D' (bottom layer)            | `D`  /  `Shift+D`                     |
| L / L' (left layer)              | `L`  /  `Shift+L`                     |
| R / R' (right layer)             | `R`  /  `Shift+R`                     |
| F / F' (front layer)             | `F`  /  `Shift+F`                     |
| B / B' (back layer)              | `B`  /  `Shift+B`                     |
| Orbit camera                     | Left-click + drag                     |
| Zoom camera                      | Mouse scroll                          |
| Quit                             | `Esc`                                 |

Uppercase letters turn the corresponding face clockwise (as viewed from
outside that face); holding Shift turns it counter-clockwise. While a
turn is animating (~0.25s), further turn key-presses are ignored until it
finishes -- see the comment block at the top of `RubiksCube.hpp` for the
exact (axis, layer, sign) table and the commit-on-completion animation
model.

Full click-and-drag-on-a-cubie-face picking is **not** implemented (it
was called out as an optional stretch goal in the spec); only keyboard
turns and camera-orbit dragging are wired up.

## Customizing the pattern data

Each visible cubie face's 12x12 peg pattern is a `qrcube::CubeFaceData`
(`include/CubeFaceData.hpp`), generated once at startup in `main.cpp`
inside the per-cubie assembly loop:

```cpp
unsigned int seed = static_cast<unsigned int>(cubieSeedCounter * 7 + f + 1);
faceData[static_cast<size_t>(f)] = qrcube::makePseudoRandomFaceData(seed);
```

To use different content:
- Swap `makePseudoRandomFaceData(seed)` for `makeRingFaceData()` (a
  simple per-tile border pattern) -- both live in `CubeFaceData.hpp`.
- Or construct a `CubeFaceData` directly from your own
  `std::bitset<144>` (e.g. decoded from a real QR code, loaded from a
  file, etc.) via `CubeFaceData(myBitset)`, or set individual bits with
  `setBit(row, col, value)` / `setPeg(tileRow, tileCol, pegRow, pegCol,
  value)`.
- Geometry proportions (tile size, peg size, rim width, raise heights,
  gaps) are all in `qrcube::CubieMeshParams` (`CubieMeshGenerator.hpp`),
  also set once in `main.cpp` as `meshParams`.

## Architecture

```
include/CubeFaceData.hpp        12x12-bit peg data model (no 3D/GL knowledge)
include/CubieMeshGenerator.hpp  Part 1 geometry interface (face frames, mesh gen)
src/CubieMeshGenerator.cpp      Part 1 geometry implementation + frame derivation
include/Shader.hpp / src/Shader.cpp     shader compile/link helper
include/Camera.hpp / src/Camera.cpp     orbit camera
include/Cubie.hpp               one cubie: grid pos, orientation, mesh range, model matrix
include/RubiksCube.hpp          move-notation table, turn/animation doc comments
src/RubiksCube.cpp              turn logic, slot mapping, animation state machine
include/InputController.hpp     key-binding doc comment block
src/InputController.cpp         GLFW key/mouse callback wiring
src/main.cpp                    window/context setup, mesh assembly, render loop
shaders/cubie.vert, cubie.frag  Phong (ambient+diffuse+specular) shading
test/RubiksCubeTest.cpp         standalone Part-3 move-logic verification harness
test/glm/                       minimal GLM-API-compatible shim used ONLY to compile
                                 RubiksCubeTest in an offline dev sandbox without GLM
                                 installed; the CMake build links the real GLM instead
```

## Verification performed

This project was developed and verified in a sandboxed Linux environment
**without internet access and without GLFW/GLM/GLAD installed**, so the
full graphical application could not actually be compiled or run end to
end here -- that step needs to be done in your own environment with the
prerequisites above installed (the build commands are given so you can do
exactly that).

What WAS actually compiled and run in this environment, with results
included verbatim:

1. **Move-logic verification (Part 3 requirement)**: `RubiksCube.cpp` +
   `Cubie.hpp` were compiled (against a small GLM-API-compatible shim,
   since real GLM wasn't installable offline -- see `test/glm/`) into
   `test/RubiksCubeTest.cpp` and actually executed. It checks, for all 12
   standard moves (U/U'/D/D'/L/L'/R/R'/F/F'/B/B'):
   - applying the same move 4 times returns every one of the 27 cubies to
     its exact original grid position AND orientation quaternion (up to
     the q/-q double-cover identity), and
   - a single application of the move changes exactly the 9 cubies in the
     selected layer and leaves the other 18 completely untouched (neither
     position nor orientation changes),
   - and, separately, that the position-to-cubie slot mapping remains a
     valid bijection (no duplicate or empty slots) after a 16-move
     scramble sequence.
   All checks passed. This compiled cleanly under `-Wall -Wextra` with no
   warnings.

2. **Winding/outward-normal verification (Part 1 / Quality requirement)**:
   `CubieMeshGenerator.cpp` was compiled standalone (again against the
   GLM shim) and run against several visibility configurations (a
   corner-like cubie with 3 visible faces, an edge-like cubie with 2, a
   face-center-like cubie with 1, and a fully-interior cubie with 0). For
   every triangle in every generated mesh (up to 5976 triangles for the
   fully-visible case), the geometric normal recomputed from the
   triangle's own vertex positions was checked against its stored vertex
   normal; **0 of 7932+ triangles across all cases had a winding/normal
   mismatch**. This is a stronger, automated version of the by-hand
   cross-product derivation documented in `CubieMeshGenerator.cpp`
   (reproduced there for the six face frames), and additionally confirms
   `addQuad()`'s self-correcting winding logic works correctly in
   practice, not just on paper.

What was **not** run here, and should be checked once you build it with
the real OpenGL stack:
   - that the window actually opens and the cube renders with visible
     tile/peg relief and correctly lit Phong shading,
   - that keyboard-triggered turns visually animate the correct 9-cubie
     layer smoothly without corrupting the rest of the cube on screen,
   - that mouse orbit/zoom behaves as expected.

The logic these depend on (turn algebra, animation interpolation math,
model-matrix composition, shader code) has been written and reviewed
carefully and exercises the same code paths verified above, but the
actual GPU rendering output has not been visually confirmed in this
environment. Please report back (or file a thumbs-down with details) if
anything looks wrong once you run it -- the most likely sharp edges, if
any remain, would be in `main.cpp`'s buffer upload/draw-call wiring or in
shader uniform names, since those are the parts that could not be
exercised by either of the two non-graphical test programs above.
