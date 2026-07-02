// Shader.hpp
// Small helper for compiling/linking a vertex+fragment shader program and
// setting uniforms by name.

#pragma once

#include <string>

#include <glm/glm.hpp>

namespace qrcube {

class Shader {
public:
    Shader() = default;
    // Loads, compiles, and links a vertex+fragment shader pair from disk.
    // Throws std::runtime_error on compile/link failure (with the GL log).
    Shader(const std::string& vertexPath, const std::string& fragmentPath);
    ~Shader();

    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;
    Shader(Shader&& other) noexcept;
    Shader& operator=(Shader&& other) noexcept;

    void use() const;
    unsigned int id() const { return programId_; }

    void setMat4(const std::string& name, const glm::mat4& value) const;
    void setVec3(const std::string& name, const glm::vec3& value) const;
    void setFloat(const std::string& name, float value) const;
    void setInt(const std::string& name, int value) const;

private:
    unsigned int programId_ = 0;

    static std::string readFile(const std::string& path);
    static unsigned int compile(unsigned int type, const std::string& source, const std::string& debugName);
};

} // namespace qrcube
