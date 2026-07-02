// TEST-ONLY minimal GLM-compatible shim. See file header comment above.
#pragma once
#include <cmath>

namespace glm {

struct ivec3 {
    int x = 0, y = 0, z = 0;
    ivec3() = default;
    ivec3(int x_, int y_, int z_) : x(x_), y(y_), z(z_) {}
    bool operator==(const ivec3& o) const { return x == o.x && y == o.y && z == o.z; }
    bool operator!=(const ivec3& o) const { return !(*this == o); }
};

struct vec3 {
    float x = 0, y = 0, z = 0;
    vec3() = default;
    vec3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}
    explicit vec3(float s) : x(s), y(s), z(s) {}
    vec3(const ivec3& v) : x(static_cast<float>(v.x)), y(static_cast<float>(v.y)), z(static_cast<float>(v.z)) {}
};

inline vec3 operator*(const vec3& v, float s) { return vec3(v.x * s, v.y * s, v.z * s); }
inline vec3 operator*(float s, const vec3& v) { return v * s; }
inline vec3 operator-(const vec3& v) { return vec3(-v.x, -v.y, -v.z); }
inline vec3 operator+(const vec3& a, const vec3& b) { return vec3(a.x + b.x, a.y + b.y, a.z + b.z); }
inline vec3 operator-(const vec3& a, const vec3& b) { return vec3(a.x - b.x, a.y - b.y, a.z - b.z); }

inline float dot(const vec3& a, const vec3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline vec3 cross(const vec3& a, const vec3& b) {
    return vec3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
}
inline float length(const vec3& v) { return std::sqrt(dot(v, v)); }
inline vec3 normalize(const vec3& v) {
    float l = length(v);
    return l > 1e-12f ? v * (1.0f / l) : v;
}

// 3x3 matrix, row-major storage for simplicity (m[row][col]).
struct mat3 {
    float m[3][3] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
};
inline vec3 operator*(const mat3& M, const vec3& v) {
    return vec3(M.m[0][0] * v.x + M.m[0][1] * v.y + M.m[0][2] * v.z,
                M.m[1][0] * v.x + M.m[1][1] * v.y + M.m[1][2] * v.z,
                M.m[2][0] * v.x + M.m[2][1] * v.y + M.m[2][2] * v.z);
}

// 4x4 matrix, row-major storage (m[row][col]). mat4(1.0f) => identity.
struct mat4 {
    float m[4][4] = {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 1}};
    mat4() = default;
    explicit mat4(float diag) {
        for (int r = 0; r < 4; ++r)
            for (int c = 0; c < 4; ++c) m[r][c] = (r == c) ? diag : 0.0f;
    }
};
inline mat4 operator*(const mat4& A, const mat4& B) {
    mat4 R(0.0f);
    for (int r = 0; r < 4; ++r)
        for (int c = 0; c < 4; ++c) {
            float sum = 0.0f;
            for (int k = 0; k < 4; ++k) sum += A.m[r][k] * B.m[k][c];
            R.m[r][c] = sum;
        }
    return R;
}

struct quat {
    float w = 1, x = 0, y = 0, z = 0;
    quat() = default;
    quat(float w_, float x_, float y_, float z_) : w(w_), x(x_), y(y_), z(z_) {}
};
inline quat operator*(const quat& a, const quat& b) {
    return quat(
        a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z,
        a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
        a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
        a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w);
}
inline float length(const quat& q) { return std::sqrt(q.w * q.w + q.x * q.x + q.y * q.y + q.z * q.z); }
inline quat normalize(const quat& q) {
    float l = length(q);
    if (l < 1e-12f) return q;
    return quat(q.w / l, q.x / l, q.y / l, q.z / l);
}

inline quat angleAxis(float angleRadians, const vec3& axisIn) {
    vec3 axis = normalize(axisIn);
    float half = angleRadians * 0.5f;
    float s = std::sin(half);
    return quat(std::cos(half), axis.x * s, axis.y * s, axis.z * s);
}

inline mat3 mat3_cast(const quat& qIn) {
    quat q = normalize(qIn);
    mat3 R;
    float xx = q.x * q.x, yy = q.y * q.y, zz = q.z * q.z;
    float xy = q.x * q.y, xz = q.x * q.z, yz = q.y * q.z;
    float wx = q.w * q.x, wy = q.w * q.y, wz = q.w * q.z;
    R.m[0][0] = 1 - 2 * (yy + zz); R.m[0][1] = 2 * (xy - wz);     R.m[0][2] = 2 * (xz + wy);
    R.m[1][0] = 2 * (xy + wz);     R.m[1][1] = 1 - 2 * (xx + zz); R.m[1][2] = 2 * (yz - wx);
    R.m[2][0] = 2 * (xz - wy);     R.m[2][1] = 2 * (yz + wx);     R.m[2][2] = 1 - 2 * (xx + yy);
    return R;
}

inline mat4 mat4_cast(const quat& q) {
    mat3 r3 = mat3_cast(q);
    mat4 R(1.0f);
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j) R.m[i][j] = r3.m[i][j];
    return R;
}

inline mat4 translate(const mat4& M, const vec3& v) {
    mat4 T(1.0f);
    T.m[0][3] = v.x; T.m[1][3] = v.y; T.m[2][3] = v.z;
    return M * T;
}

inline mat4 rotate(const mat4& M, float angleRadians, const vec3& axisIn) {
    quat q = angleAxis(angleRadians, axisIn);
    return M * mat4_cast(q);
}

template <typename T>
inline T half_pi() { return static_cast<T>(1.5707963267948966); }

template <typename T>
inline T clamp(T v, T lo, T hi) { return v < lo ? lo : (v > hi ? hi : v); }

} // namespace glm
