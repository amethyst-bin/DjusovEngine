#pragma once

#include "render/Mesh.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace Djusov {

class Shader;

struct BoneInfo {
    int id = -1;
    std::string name;
    int parentId = -1;
    glm::mat4 inverseBindMatrix = glm::mat4(1.0f);
    glm::mat4 localTransform = glm::mat4(1.0f);
    glm::mat4 defaultLocalTransform = glm::mat4(1.0f);
};

class SkeletalMesh {
public:
    SkeletalMesh();
    ~SkeletalMesh();

    bool loadFBX(const std::string& filePath);
    void updateAnimation(float dt, float speedFactor, float adsProgress, float recoilProgress);
    void setBoneRotation(const std::string& name, const glm::quat& rot);
    void resetPose();

    void draw(const Shader& shader) const;

    bool isValid() const { return m_mesh != nullptr; }
    const std::vector<glm::mat4>& getBoneMatrices() const { return m_finalBoneMatrices; }
    size_t getBoneCount() const { return m_bones.size(); }

private:
    void updateGlobalBoneMatrices();

    std::shared_ptr<Mesh> m_mesh;
    std::vector<BoneInfo> m_bones;
    std::unordered_map<std::string, int> m_boneNameToId;
    std::vector<glm::mat4> m_finalBoneMatrices;

    float m_animTime = 0.0f;
};

} // namespace Djusov
