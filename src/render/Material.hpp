#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include <nlohmann/json.hpp>

namespace Djusov {

class Shader;

struct Material {
    std::string name = "Default";
    glm::vec3 albedo = glm::vec3(0.8f, 0.8f, 0.8f);
    float metallic = 0.0f;
    float roughness = 0.5f;
    float ao = 1.0f;
    glm::vec3 emissive = glm::vec3(0.0f);
    float emissiveIntensity = 1.0f;
    float transmission = 0.0f;
    float ior = 1.52f;
    float alpha = 1.0f;
    bool isMirror = false;
    bool isGlass = false;
    glm::vec2 uvTiling = glm::vec2(1.0f, 1.0f);
    glm::vec2 uvOffset = glm::vec2(0.0f, 0.0f);

    std::string albedoTexturePath = "";
    std::string normalTexturePath = "";
    unsigned int albedoTextureId = 0;
    unsigned int normalTextureId = 0;

    void apply(const Shader& shader) const;
    nlohmann::json toJson() const;
    static Material fromJson(const nlohmann::json& j);
};

class MaterialManager {
public:
    static void init();
    static void addMaterial(const Material& mat);
    static Material* getMaterial(const std::string& name);
    static const std::unordered_map<std::string, Material>& getAllMaterials();
    static std::vector<std::string> getMaterialNames();

    // Factory methods for the 12 built-in presets
    static Material createAsphalt();
    static Material createWood();
    static Material createWoodPlanks();
    static Material createGlass();
    static Material createMirror();
    static Material createMetal();
    static Material createDiamondPlate();
    static Material createConcrete();
    static Material createBrick();
    static Material createGrass();
    static Material createSmoothPlastic();
    static Material createNeon();

private:
    static std::unordered_map<std::string, Material> s_materials;
};

} // namespace Djusov
