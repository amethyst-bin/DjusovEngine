#include "render/Mesh.hpp"
#include "ufbx.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;

namespace Djusov {

Mesh::Mesh(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices)
    : m_vao(0), m_vbo(0), m_ebo(0), m_indexCount(0) {
    init(vertices, indices);
}

Mesh::~Mesh() {
    cleanup();
}

void Mesh::cleanup() {
    if (glfwGetCurrentContext() != nullptr) {
        if (m_ebo) { glDeleteBuffers(1, &m_ebo); m_ebo = 0; }
        if (m_vbo) { glDeleteBuffers(1, &m_vbo); m_vbo = 0; }
        if (m_vao) { glDeleteVertexArrays(1, &m_vao); m_vao = 0; }
    } else {
        m_ebo = m_vbo = m_vao = 0;
    }
    m_indexCount = 0;
}

void Mesh::init(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices) {
    cleanup();
    m_vertices = vertices;
    m_indices = indices;
    m_indexCount = indices.size();

    if (glfwGetCurrentContext() == nullptr) {
        return; // Headless / no OpenGL context active
    }

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glGenBuffers(1, &m_ebo);

    glBindVertexArray(m_vao);

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    // 0: Position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));

    // 1: Normal
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));

    // 2: TexCoords
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoords));

    // 3: Tangent
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, tangent));

    // 4: Bone IDs (ivec4)
    glEnableVertexAttribArray(4);
    glVertexAttribIPointer(4, 4, GL_INT, sizeof(Vertex), (void*)offsetof(Vertex, boneIds));

    // 5: Bone Weights (vec4)
    glEnableVertexAttribArray(5);
    glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, boneWeights));

    glBindVertexArray(0);
}

void Mesh::draw() const {
    if (m_vao != 0 && m_indexCount > 0) {
        glBindVertexArray(m_vao);
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(m_indexCount), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }
}

std::shared_ptr<Mesh> Mesh::loadModel(const std::string& filePath, float targetLength) {
    std::string path = filePath;
    if (!fs::exists(path)) {
        if (fs::exists("assets/" + path)) path = "assets/" + path;
        else if (fs::exists("../" + path)) path = "../" + path;
        else return nullptr;
    }

    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    std::string ext = fs::path(path).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    if (ext == ".fbx") {
        ufbx_load_opts opts = { 0 };
        opts.target_axes = ufbx_axes_right_handed_y_up;
        opts.target_unit_meters = 1.0f;
        ufbx_error error;
        ufbx_scene* scene = ufbx_load_file(path.c_str(), &opts, &error);
        if (!scene) {
            std::cerr << "[Mesh] Failed to load FBX model '" << path << "': " << error.description.data << std::endl;
            return nullptr;
        }

        for (size_t m = 0; m < scene->meshes.count; ++m) {
            ufbx_mesh* ufbxMesh = scene->meshes.data[m];
            ufbx_matrix transform = ufbx_identity_matrix;
            if (ufbxMesh->instances.count > 0 && ufbxMesh->instances.data[0]) {
                transform = ufbxMesh->instances.data[0]->node_to_world;
            }

            for (size_t faceIdx = 0; faceIdx < ufbxMesh->num_faces; ++faceIdx) {
                ufbx_face face = ufbxMesh->faces.data[faceIdx];
                size_t numTri = face.num_indices - 2;

                for (size_t t = 0; t < numTri; ++t) {
                    uint32_t cornerIndices[3] = {
                        face.index_begin + 0,
                        face.index_begin + static_cast<uint32_t>(t + 1),
                        face.index_begin + static_cast<uint32_t>(t + 2)
                    };

                    for (int c = 0; c < 3; ++c) {
                        uint32_t cIdx = cornerIndices[c];
                        uint32_t vIdx = ufbxMesh->vertex_indices.data[cIdx];

                        Vertex v;
                        ufbx_vec3 pos = ufbxMesh->vertices.data[vIdx];
                        ufbx_vec3 transPos = ufbx_transform_position(&transform, pos);
                        v.position = glm::vec3(transPos.x, transPos.y, transPos.z);

                        if (ufbxMesh->vertex_normal.exists) {
                            ufbx_vec3 norm = ufbx_get_vertex_vec3(&ufbxMesh->vertex_normal, cIdx);
                            ufbx_vec3 transNorm = ufbx_transform_direction(&transform, norm);
                            v.normal = glm::normalize(glm::vec3(transNorm.x, transNorm.y, transNorm.z));
                        } else {
                            v.normal = glm::vec3(0, 1, 0);
                        }

                        if (ufbxMesh->vertex_uv.exists) {
                            ufbx_vec2 uv = ufbx_get_vertex_vec2(&ufbxMesh->vertex_uv, cIdx);
                            v.texCoords = glm::vec2(uv.x, uv.y);
                        }

                        indices.push_back(static_cast<unsigned int>(vertices.size()));
                        vertices.push_back(v);
                    }
                }
            }
        }
        ufbx_free_scene(scene);
    } else if (ext == ".obj") {
        std::ifstream file(path);
        if (!file.is_open()) return nullptr;

        std::vector<glm::vec3> temp_positions;
        std::string line;
        while (std::getline(file, line)) {
            if (line.rfind("v ", 0) == 0) {
                std::istringstream s(line.substr(2));
                glm::vec3 p;
                s >> p.x >> p.y >> p.z;
                temp_positions.push_back(p);
            } else if (line.rfind("f ", 0) == 0) {
                std::istringstream s(line.substr(2));
                std::string v1, v2, v3;
                s >> v1 >> v2 >> v3;
                auto parseIndex = [](const std::string& token) {
                    size_t slash = token.find('/');
                    std::string idxStr = (slash != std::string::npos) ? token.substr(0, slash) : token;
                    return std::stoi(idxStr) - 1;
                };
                int i1 = parseIndex(v1);
                int i2 = parseIndex(v2);
                int i3 = parseIndex(v3);
                if (i1 >= 0 && i1 < (int)temp_positions.size() &&
                    i2 >= 0 && i2 < (int)temp_positions.size() &&
                    i3 >= 0 && i3 < (int)temp_positions.size()) {
                    Vertex vert1, vert2, vert3;
                    vert1.position = temp_positions[i1];
                    vert2.position = temp_positions[i2];
                    vert3.position = temp_positions[i3];

                    glm::vec3 normal = glm::normalize(glm::cross(vert2.position - vert1.position, vert3.position - vert1.position));
                    vert1.normal = vert2.normal = vert3.normal = normal;

                    indices.push_back(static_cast<unsigned int>(vertices.size()));
                    vertices.push_back(vert1);
                    indices.push_back(static_cast<unsigned int>(vertices.size()));
                    vertices.push_back(vert2);
                    indices.push_back(static_cast<unsigned int>(vertices.size()));
                    vertices.push_back(vert3);
                }
            }
        }
    }

    if (vertices.empty()) return nullptr;

    // Normalize bounds if targetLength is requested
    if (targetLength > 0.0f) {
        glm::vec3 minP(1e9f), maxP(-1e9f);
        for (const auto& v : vertices) {
            minP = glm::min(minP, v.position);
            maxP = glm::max(maxP, v.position);
        }
        glm::vec3 size = maxP - minP;
        float maxDim = std::max({ size.x, size.y, size.z });
        if (maxDim > 0.0001f) {
            float scaleFactor = targetLength / maxDim;
            glm::vec3 center = (minP + maxP) * 0.5f;

            // Orient model so longest axis extends along -Z (barrel forward)
            bool lengthIsX = (size.x >= size.y && size.x >= size.z);
            bool lengthIsY = (size.y >= size.x && size.y >= size.z);

            for (auto& v : vertices) {
                glm::vec3 p = (v.position - center) * scaleFactor;
                if (lengthIsX) {
                    p = glm::vec3(p.y, p.z, -p.x);
                } else if (lengthIsY) {
                    p = glm::vec3(p.x, p.z, -p.y);
                }
                v.position = p;
            }
        }
    }

    return std::make_shared<Mesh>(vertices, indices);
}

} // namespace Djusov
