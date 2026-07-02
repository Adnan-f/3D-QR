#include "ObjExporter.h"

#include <fstream>
#include <iomanip>

namespace cube3d {

bool exportObj(const Mesh& mesh, const std::string& path) {
    std::ofstream out(path);
    if (!out.is_open()) return false;

    out << std::fixed << std::setprecision(6);
    for (const Vec3& v : mesh.vertices()) {
        out << "v " << v.x << ' ' << v.y << ' ' << v.z << '\n';
    }
    for (const Triangle& t : mesh.triangles()) {
        // OBJ face indices are 1-based; Mesh stores 0-based indices.
        // This is the one and only place that +1 conversion happens.
        out << "f " << (t.a + 1) << ' ' << (t.b + 1) << ' ' << (t.c + 1) << '\n';
    }
    return out.good();
}

} // namespace cube3d
