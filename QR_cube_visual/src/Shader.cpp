// Shader.cpp
#include "Shader.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <utility>

#include <glad/glad.h>

namespace qrcube {

std::string Shader::readFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Shader: could not open file: " + path);
    }
    std::stringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

unsigned int Shader::compile(unsigned int type, const std::string& source, const std::string& debugName) {
    unsigned int shader = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    int success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[1024];
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        glDeleteShader(shader);
        throw std::runtime_error("Shader compile error (" + debugName + "): " + std::string(log));
    }
    return shader;
}

Shader::Shader(const std::string& vertexPath, const std::string& fragmentPath) {
    std::string vSrc = readFile(vertexPath);
    std::string fSrc = readFile(fragmentPath);

    unsigned int vShader = compile(GL_VERTEX_SHADER, vSrc, vertexPath);
    unsigned int fShader = compile(GL_FRAGMENT_SHADER, fSrc, fragmentPath);

    programId_ = glCreateProgram();
    glAttachShader(programId_, vShader);
    glAttachShader(programId_, fShader);
    glLinkProgram(programId_);

    int success = 0;
    glGetProgramiv(programId_, GL_LINK_STATUS, &success);
    if (!success) {
        char log[1024];
        glGetProgramInfoLog(programId_, sizeof(log), nullptr, log);
        glDeleteShader(vShader);
        glDeleteShader(fShader);
        throw std::runtime_error("Shader link error: " + std::string(log));
    }

    glDeleteShader(vShader);
    glDeleteShader(fShader);
}

Shader::~Shader() {
    if (programId_ != 0) {
        glDeleteProgram(programId_);
    }
}

Shader::Shader(Shader&& other) noexcept : programId_(other.programId_) {
    other.programId_ = 0;
}

Shader& Shader::operator=(Shader&& other) noexcept {
    if (this != &other) {
        if (programId_ != 0) glDeleteProgram(programId_);
        programId_ = other.programId_;
        other.programId_ = 0;
    }
    return *this;
}

void Shader::use() const { glUseProgram(programId_); }

void Shader::setMat4(const std::string& name, const glm::mat4& value) const {
    int loc = glGetUniformLocation(programId_, name.c_str());
    glUniformMatrix4fv(loc, 1, GL_FALSE, &value[0][0]);
}

void Shader::setVec3(const std::string& name, const glm::vec3& value) const {
    int loc = glGetUniformLocation(programId_, name.c_str());
    glUniform3fv(loc, 1, &value[0]);
}

void Shader::setFloat(const std::string& name, float value) const {
    int loc = glGetUniformLocation(programId_, name.c_str());
    glUniform1f(loc, value);
}

void Shader::setInt(const std::string& name, int value) const {
    int loc = glGetUniformLocation(programId_, name.c_str());
    glUniform1i(loc, value);
}

} // namespace qrcube
