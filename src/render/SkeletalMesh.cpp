#include "render/SkeletalMesh.hpp"
#include "render/Shader.hpp"
#include "vendor/ufbx/ufbx.h"
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace Djusov {

static glm::mat4 toGlmMat4(const ufbx_matrix& m) {
    return glm::mat4(
        static_cast<float>(m.cols[0].x), static_cast<float>(m.cols[0].y), static_cast<float>(m.cols[0].z), 0.0f,
        static_cast<float>(m.cols[1].x), static_cast<float>(m.cols[1].y), static_cast<float>(m.cols[1].z), 0.0f,
        static_cast<float>(m.cols[2].x), static_cast<float>(m.cols[2].y), static_cast<float>(m.cols[2].z), 0.0f,
        static_cast<float>(m.cols[3].x), static_cast<float>(m.cols[3].y), static_cast<float>(m.cols[3].z), 1.0f
    );
}

SkeletalMesh::SkeletalMesh() {
    m_finalBoneMatrices.resize(64, glm::mat4(1.0f));
}

SkeletalMesh::~SkeletalMesh() {}

bool SkeletalMesh::loadFBX(const std::string& filePath) {
    ufbx_load_opts opts = { 0 };
    opts.target_axes = ufbx_axes_right_handed_y_up;
    opts.target_unit_meters = 1.0f;

    ufbx_error error;
    ufbx_scene* scene = ufbx_load_file(filePath.c_str(), &opts, &error);
    if (!scene) {
        std::cerr << "[SkeletalMesh] Failed to load FBX: " << error.description.data << std::endl;
        return false;
    }

    if (scene->meshes.count == 0) {
        std::cerr << "[SkeletalMesh] No meshes found in FBX: " << filePath << std::endl;
        ufbx_free_scene(scene);
        return false;
    }

    ufbx_mesh* ufbxMesh = scene->meshes.data[0];

    // Find skin deformer
    ufbx_skin_deformer* skin = nullptr;
    if (ufbxMesh->skin_deformers.count > 0) {
        skin = ufbxMesh->skin_deformers.data[0];
    }

    m_bones.clear();
    m_boneNameToId.clear();

    if (skin) {
        m_bones.resize(skin->clusters.count);
        for (size_t i = 0; i < skin->clusters.count; ++i) {
            ufbx_skin_cluster* cluster = skin->clusters.data[i];
            BoneInfo info;
            info.id = static_cast<int>(i);
            info.name = cluster->bone_node ? cluster->bone_node->name.data : ("Bone_" + std::to_string(i));
            info.inverseBindMatrix = toGlmMat4(cluster->geometry_to_bone);
            if (cluster->bone_node) {
                info.localTransform = toGlmMat4(cluster->bone_node->node_to_parent);
                info.defaultLocalTransform = info.localTransform;
            }
            m_bones[i] = info;
            m_boneNameToId[info.name] = static_cast<int>(i);
        }

        // Establish parent-child links
        for (size_t i = 0; i < skin->clusters.count; ++i) {
            ufbx_skin_cluster* cluster = skin->clusters.data[i];
            if (cluster->bone_node && cluster->bone_node->parent) {
                auto it = m_boneNameToId.find(cluster->bone_node->parent->name.data);
                if (it != m_boneNameToId.end()) {
                    m_bones[i].parentId = it->second;
                }
            }
        }
    }

    // Build geometry buffers
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    size_t numTriangles = ufbxMesh->num_triangles;
    vertices.reserve(numTriangles * 3);
    indices.reserve(numTriangles * 3);

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
                v.position = glm::vec3(pos.x, pos.y, pos.z);

                if (ufbxMesh->vertex_normal.exists) {
                    ufbx_vec3 norm = ufbx_get_vertex_vec3(&ufbxMesh->vertex_normal, cIdx);
                    v.normal = glm::vec3(norm.x, norm.y, norm.z);
                }

                if (ufbxMesh->vertex_uv.exists) {
                    ufbx_vec2 uv = ufbx_get_vertex_vec2(&ufbxMesh->vertex_uv, cIdx);
                    v.texCoords = glm::vec2(uv.x, uv.y);
                }

                if (ufbxMesh->vertex_tangent.exists) {
                    ufbx_vec3 tan = ufbx_get_vertex_vec3(&ufbxMesh->vertex_tangent, cIdx);
                    v.tangent = glm::vec3(tan.x, tan.y, tan.z);
                }

                // Skinning weights
                if (skin && vIdx < skin->vertices.count) {
                    ufbx_skin_vertex svert = skin->vertices.data[vIdx];
                    float totalWeight = 0.0f;
                    int count = std::min(static_cast<int>(svert.num_weights), 4);

                    for (int w = 0; w < count; ++w) {
                        ufbx_skin_weight sw = skin->weights.data[svert.weight_begin + w];
                        v.boneIds[w] = static_cast<int>(sw.cluster_index);
                        v.boneWeights[w] = static_cast<float>(sw.weight);
                        totalWeight += v.boneWeights[w];
                    }
                    if (totalWeight > 0.0001f) {
                        v.boneWeights /= totalWeight; // normalize
                    }
                }

                indices.push_back(static_cast<unsigned int>(vertices.size()));
                vertices.push_back(v);
            }
        }
    }

    m_mesh = std::make_shared<Mesh>(vertices, indices);
    m_finalBoneMatrices.resize(std::max(m_bones.size(), size_t(64)), glm::mat4(1.0f));

    updateGlobalBoneMatrices();
    ufbx_free_scene(scene);

    std::cout << "[SkeletalMesh] Loaded rigged model '" << filePath 
              << "' with " << m_bones.size() << " bones and " 
              << vertices.size() << " vertices." << std::endl;
    return true;
}

void SkeletalMesh::resetPose() {
    for (auto& b : m_bones) {
        b.localTransform = b.defaultLocalTransform;
    }
    updateGlobalBoneMatrices();
}

void SkeletalMesh::setBoneRotation(const std::string& name, const glm::quat& rot) {
    auto it = m_boneNameToId.find(name);
    if (it != m_boneNameToId.end()) {
        int id = it->second;
        glm::vec3 translation = glm::vec3(m_bones[id].defaultLocalTransform[3]);
        glm::mat4 tMat = glm::translate(glm::mat4(1.0f), translation);
        glm::mat4 rMat = glm::mat4_cast(rot);
        m_bones[id].localTransform = tMat * rMat;
    }
}

void SkeletalMesh::updateAnimation(float dt, float speedFactor, float adsProgress, float recoilProgress) {
    m_animTime += dt * (1.0f + speedFactor * 3.0f);

    // Natural procedural hand animation
    // Sway & breathing
    float breathe = std::sin(m_animTime * 1.5f) * 0.015f;
    float stepSway = std::sin(m_animTime * 4.0f) * 0.035f * speedFactor;

    for (size_t i = 0; i < m_bones.size(); ++i) {
        m_bones[i].localTransform = m_bones[i].defaultLocalTransform;

        // Apply procedural adjustments based on bone name
        const std::string& name = m_bones[i].name;

        // Left arm support
        if (name.find("Left") != std::string::npos || name.find("left") != std::string::npos || name.find(".L") != std::string::npos) {
            glm::mat4 mod = glm::rotate(glm::mat4(1.0f), breathe + stepSway * 0.5f, glm::vec3(0, 0, 1));
            m_bones[i].localTransform = m_bones[i].localTransform * mod;
        }

        // Right arm & grip
        if (name.find("Right") != std::string::npos || name.find("right") != std::string::npos || name.find(".R") != std::string::npos) {
            float recoilAngle = recoilProgress * 0.12f;
            glm::mat4 mod = glm::rotate(glm::mat4(1.0f), -recoilAngle, glm::vec3(1, 0, 0));
            m_bones[i].localTransform = m_bones[i].localTransform * mod;
        }
    }

    updateGlobalBoneMatrices();
}

void SkeletalMesh::updateGlobalBoneMatrices() {
    std::vector<glm::mat4> worldTransforms(m_bones.size(), glm::mat4(1.0f));
    std::vector<bool> computed(m_bones.size(), false);

    auto computeWorld = [&](auto& self, size_t idx) -> glm::mat4 {
        if (computed[idx]) return worldTransforms[idx];
        int parent = m_bones[idx].parentId;
        if (parent >= 0 && parent < static_cast<int>(m_bones.size()) && static_cast<size_t>(parent) != idx) {
            worldTransforms[idx] = self(self, static_cast<size_t>(parent)) * m_bones[idx].localTransform;
        } else {
            worldTransforms[idx] = m_bones[idx].localTransform;
        }
        computed[idx] = true;
        return worldTransforms[idx];
    };

    for (size_t i = 0; i < m_bones.size(); ++i) {
        computeWorld(computeWorld, i);
        m_finalBoneMatrices[i] = worldTransforms[i] * m_bones[i].inverseBindMatrix;
    }
}

void SkeletalMesh::draw(const Shader& shader) const {
    if (!m_mesh) return;

    shader.setBool("uHasSkinning", true);
    for (size_t i = 0; i < std::min(m_finalBoneMatrices.size(), size_t(64)); ++i) {
        std::string uniformName = "uBones[" + std::to_string(i) + "]";
        shader.setMat4(uniformName, m_finalBoneMatrices[i]);
    }

    m_mesh->draw();
}

} // namespace Djusov
