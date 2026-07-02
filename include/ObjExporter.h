#pragma once
#include "Mesh.h"
#include <string>

namespace cube3d {

// Writes `mesh` to `path` as a Wavefront OBJ file ("v" lines, then "f"
// lines with 1-based indices). Returns true on success. The 0-based ->
// 1-based index conversion happens in exactly one place: inside this
// function, at export time -- the Mesh itself always stores 0-based
// indices internally.
bool exportObj(const Mesh& mesh, const std::string& path);

} // namespace cube3d
