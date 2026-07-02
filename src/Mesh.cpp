#include "Mesh.h"
#include <cmath>

namespace cube3d {

Vec3 operator+(const Vec3& a, const Vec3& b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
Vec3 operator-(const Vec3& a, const Vec3& b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
Vec3 operator*(const Vec3& a, double s) { return {a.x * s, a.y * s, a.z * s}; }

Vec3 cross(const Vec3& a, const Vec3& b) {
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

double dot(const Vec3& a, const Vec3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

double length(const Vec3& a) { return std::sqrt(dot(a, a)); }

int Mesh::addVertex(const Vec3& p) {
    vertices_.push_back(p);
    return static_cast<int>(vertices_.size()) - 1;
}

void Mesh::addTriangle(int a, int b, int c) {
    triangles_.push_back(Triangle{a, b, c});
}

void Mesh::addQuad(const Vec3& p0, const Vec3& p1, const Vec3& p2, const Vec3& p3) {
    int i0 = addVertex(p0);
    int i1 = addVertex(p1);
    int i2 = addVertex(p2);
    int i3 = addVertex(p3);
    addTriangle(i0, i1, i2);
    addTriangle(i0, i2, i3);
}

} // namespace cube3d
