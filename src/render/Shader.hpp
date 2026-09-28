#pragma once

#include <string>
#include <unordered_map>
#include <GL/glew.h>
#include <glm/glm.hpp>

namespace Djusov {

class Shader {
public:
    Shader() : m_program(0) {}
    ~Shader();

    bool loadFromFiles(const std::string& vertexPath, const std::string& fragmentPath);
    bool loadFromSource(const std::string& vertexSrc, const std::string& fragmentSrc);

    void use() const;
    void unuse() const;

    GLuint getProgram() const { return m_program; }
    bool isValid() const { return m_program != 0; }

    // Uniform setters
    void setBool(const std::string& name, bool value) const;
    void setInt(const std::string& name, int value) const;
    void setFloat(const std::string& name, float value) const;
    void setVec2(const std::string& name, const glm::vec2& value) const;
    void setVec3(const std::string& name, const glm::vec3& value) const;
    void setVec4(const std::string& name, const glm::vec4& value) const;
    void setMat3(const std::string& name, const glm::mat3& value) const;
    void setMat4(const std::string& name, const glm::mat4& value) const;

private:
    GLuint m_program;
    mutable std::unordered_map<std::string, GLint> m_uniformLocationCache;

    GLint getUniformLocation(const std::string& name) const;
    GLuint compileShader(GLenum type, const std::string& source);
};

} // namespace Djusov
