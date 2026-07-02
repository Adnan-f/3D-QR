# Procedural 3D Tiled-Cube Pattern Generator (C++17)

Generates a cube where each of the 6 faces shows a 3x3 grid of raised
tiles, each tile having a rim + a 4x4 grid of pegs (864 bits total,
`std::bitset<144>` per face), and exports it to `cube.obj`.

## Build & run

```bash
g++ -std=c++17 -Wall -Wextra -I include src/*.cpp -o cube3d_demo
./cube3d_demo
```

Compiles with zero warnings. Running it writes `cube.obj` to the
current directory and prints vertex/triangle/bit stats plus built-in
verification (face-frame winding, independent vertex/triangle count
formula, divergence-theorem signed-volume check).

## Where to edit the data

All hardcoded demo bit patterns live in `populateDemoPatterns()` in
`src/main.cpp`. Set bits with `grid.set(row, col, value)` (raw 12x12
indices) or `grid.setPeg(tileRow, tileCol, pegRow, pegCol, value)`
(tile/peg indices). A future automated decoder can populate the same
`CubeData` object the same way without touching `Geometry.*` or
`ObjExporter.*` at all.

## File layout

- `include/BitGrid.h`, `src/BitGrid.cpp` — pure binary data model (six
  `FaceGrid`s of 144 bits each), no 3D knowledge.
- `include/Mesh.h`, `src/Mesh.cpp` — indexed triangle mesh + the single
  `addQuad` helper every flat panel/wall/cap funnels through.
- `include/Geometry.h`, `src/Geometry.cpp` — turns bit data + params
  into a `Mesh`. Contains the six face local-frames, the winding-order
  derivation (in comments) and `verifyFaceFrames()`.
- `include/ObjExporter.h`, `src/ObjExporter.cpp` — `Mesh` -> `.obj`
  file, including the one-and-only 0-based -> 1-based index
  conversion.
- `src/main.cpp` — demo data, wiring, stats, verification.

