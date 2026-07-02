#pragma once
// Indexed triangle mesh: a flat vertex list + triangles referencing
// 0-based vertex indices. No connectivity merging/deduplication is
// performed (each addQuad call adds 4 brand-new vertices) -- this keeps
// the geometry code trivially simple at the cost of some duplicated
// vertices along seams, which is irrelevant for a procedurally generated
// export-only mesh like this one.

#include <vector>

namespace cube3d {

struct Vec3 {
    double x = 0.0, y = 0.0, z = 0.0;
};

Vec3 operator+(const Vec3& a, const Vec3& b);
Vec3 operator-(const Vec3& a, const Vec3& b);
Vec3 operator*(const Vec3& a, double s);
Vec3 cross(const Vec3& a, const Vec3& b);
double dot(const Vec3& a, const Vec3& b);
double length(const Vec3& a);

struct Triangle {
    int a = 0, b = 0, c = 0; // 0-based vertex indices
};

class Mesh {
public:
    // Adds a vertex, returns its 0-based index.
    int addVertex(const Vec3& p);

    // Adds a triangle directly from 0-based vertex indices.
    void addTriangle(int a, int b, int c);

    // Adds a planar quad as two triangles: (p0,p1,p2) and (p0,p2,p3).
    // The caller is responsible for supplying corners in a winding
    // order such that cross(p1-p0, p2-p0) already points along the
    // intended outward direction -- see Geometry.cpp for the derivation
    // of that winding order for every quad type used in this project.
    // This is the ONLY place quads are turned into triangles, so every
    // flat panel/rim/floor/cap/wall in the project funnels through here.
    void addQuad(const Vec3& p0, const Vec3& p1, const Vec3& p2, const Vec3& p3);

    const std::vector<Vec3>& vertices() const { return vertices_; }
    const std::vector<Triangle>& triangles() const { return triangles_; }

private:
    std::vector<Vec3> vertices_;
    std::vector<Triangle> triangles_;
};

} // namespace cube3d
