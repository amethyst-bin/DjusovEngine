#pragma once

#include <vector>
#include <memory>
#include <string>
#include <GL/glew.h>
#include <glm/glm.hpp>

namespace Djusov {

struct Vertex {
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 normal = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec2 texCoords = glm::vec2(0.0f);
    glm::vec3 tangent = glm::vec3(1.0f, 0.0f, 0.0f);
    glm::ivec4 boneIds = glm::ivec4(0);
    glm::vec4 boneWeights = glm::vec4(0.0f);
};

class Mesh {
public:
    Mesh() : m_vao(0), m_vbo(0), m_ebo(0), m_indexCount(0) {}
    Mesh(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices);
    ~Mesh();

    void init(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices);
    void draw() const;
    void cleanup();

    GLuint getVAO() const { return m_vao; }
    size_t getIndexCount() const { return m_indexCount; }
    const std::vector<Vertex>& getVertices() const { return m_vertices; }
    const std::vector<unsigned int>& getIndices() const { return m_indices; }

    static std::shared_ptr<Mesh> loadModel(const std::string& filePath, float targetLength = 0.0f);

private:
    GLuint m_vao;
    GLuint m_vbo;
    GLuint m_ebo;
    size_t m_indexCount;
    std::vector<Vertex> m_vertices;
    std::vector<unsigned int> m_indices;
};

} // namespace Djusov
