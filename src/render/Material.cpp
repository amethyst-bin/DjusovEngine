#include "render/Material.hpp"
#include "render/Shader.hpp"
#include <GL/glew.h>

namespace Djusov {

std::unordered_map<std::string, Material> MaterialManager::s_materials;

void Material::apply(const Shader& shader) const {
    shader.setVec3("uAlbedo", albedo);
    shader.setFloat("uMetallic", metallic);
    shader.setFloat("uRoughness", roughness);
    shader.setFloat("uAO", ao);
    shader.setVec3("uEmissive", emissive);
    shader.setFloat("uEmissiveIntensity", emissiveIntensity);
    shader.setFloat("uTransmission", transmission);
    shader.setFloat("uIOR", ior);
    shader.setFloat("uAlpha", alpha);
    shader.setBool("uIsMirror", isMirror);
    shader.setBool("uIsGlass", isGlass);
    shader.setVec2("uUVTiling", uvTiling);
    shader.setVec2("uUVOffset", uvOffset);

    // Textures
    if (albedoTextureId > 0) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, albedoTextureId);
        shader.setInt("uAlbedoMap", 0);
        shader.setBool("uUseAlbedoMap", true);
    } else {
        shader.setBool("uUseAlbedoMap", false);
    }

    if (normalTextureId > 0) {
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, normalTextureId);
        shader.setInt("uNormalMap", 1);
        shader.setBool("uUseNormalMap", true);
    } else {
        shader.setBool("uUseNormalMap", false);
    }
}

nlohmann::json Material::toJson() const {
    nlohmann::json j;
    j["name"] = name;
    j["albedo"] = { albedo.r, albedo.g, albedo.b };
    j["metallic"] = metallic;
    j["roughness"] = roughness;
    j["ao"] = ao;
    j["emissive"] = { emissive.r, emissive.g, emissive.b };
    j["emissiveIntensity"] = emissiveIntensity;
    j["transmission"] = transmission;
    j["ior"] = ior;
    j["alpha"] = alpha;
    j["isMirror"] = isMirror;
    j["isGlass"] = isGlass;
    j["uvTiling"] = { uvTiling.x, uvTiling.y };
    j["uvOffset"] = { uvOffset.x, uvOffset.y };
    j["albedoTexturePath"] = albedoTexturePath;
    j["normalTexturePath"] = normalTexturePath;
    return j;
}

Material Material::fromJson(const nlohmann::json& j) {
    Material mat;
    if (j.contains("name")) mat.name = j["name"];
    if (j.contains("albedo")) mat.albedo = glm::vec3(j["albedo"][0], j["albedo"][1], j["albedo"][2]);
    if (j.contains("metallic")) mat.metallic = j["metallic"];
    if (j.contains("roughness")) mat.roughness = j["roughness"];
    if (j.contains("ao")) mat.ao = j["ao"];
    if (j.contains("emissive")) mat.emissive = glm::vec3(j["emissive"][0], j["emissive"][1], j["emissive"][2]);
    if (j.contains("emissiveIntensity")) mat.emissiveIntensity = j["emissiveIntensity"];
    if (j.contains("transmission")) mat.transmission = j["transmission"];
    if (j.contains("ior")) mat.ior = j["ior"];
    if (j.contains("alpha")) mat.alpha = j["alpha"];
    if (j.contains("isMirror")) mat.isMirror = j["isMirror"];
    if (j.contains("isGlass")) mat.isGlass = j["isGlass"];
    if (j.contains("uvTiling")) mat.uvTiling = glm::vec2(j["uvTiling"][0], j["uvTiling"][1]);
    if (j.contains("uvOffset")) mat.uvOffset = glm::vec2(j["uvOffset"][0], j["uvOffset"][1]);
    if (j.contains("albedoTexturePath")) mat.albedoTexturePath = j["albedoTexturePath"];
    if (j.contains("normalTexturePath")) mat.normalTexturePath = j["normalTexturePath"];
    return mat;
}

// ================= Built-in Material Presets =================
Material MaterialManager::createAsphalt() {
    Material mat;
    mat.name = "Asphalt";
    mat.albedo = glm::vec3(0.18f, 0.18f, 0.20f);
    mat.metallic = 0.0f;
    mat.roughness = 0.88f;
    mat.ao = 1.0f;
    return mat;
}

Material MaterialManager::createWood() {
    Material mat;
    mat.name = "Wood";
    mat.albedo = glm::vec3(0.48f, 0.28f, 0.14f);
    mat.metallic = 0.0f;
    mat.roughness = 0.55f;
    mat.ao = 1.0f;
    return mat;
}

Material MaterialManager::createWoodPlanks() {
    Material mat;
    mat.name = "WoodPlanks";
    mat.albedo = glm::vec3(0.55f, 0.35f, 0.20f);
    mat.metallic = 0.0f;
    mat.roughness = 0.65f;
    mat.ao = 0.9f;
    mat.uvTiling = glm::vec2(2.0f, 2.0f);
    return mat;
}

Material MaterialManager::createGlass() {
    Material mat;
    mat.name = "Glass";
    mat.albedo = glm::vec3(0.85f, 0.95f, 0.98f);
    mat.metallic = 0.0f;
    mat.roughness = 0.05f;
    mat.transmission = 0.95f;
    mat.ior = 1.52f;
    mat.alpha = 0.25f;
    mat.isGlass = true;
    return mat;
}

Material MaterialManager::createMirror() {
    Material mat;
    mat.name = "Mirror";
    mat.albedo = glm::vec3(0.95f, 0.95f, 0.95f);
    mat.metallic = 1.0f;
    mat.roughness = 0.01f;
    mat.isMirror = true;
    return mat;
}

Material MaterialManager::createMetal() {
    Material mat;
    mat.name = "Metal";
    mat.albedo = glm::vec3(0.75f, 0.76f, 0.78f);
    mat.metallic = 1.0f;
    mat.roughness = 0.25f;
    return mat;
}

Material MaterialManager::createDiamondPlate() {
    Material mat;
    mat.name = "DiamondPlate";
    mat.albedo = glm::vec3(0.65f, 0.67f, 0.70f);
    mat.metallic = 0.95f;
    mat.roughness = 0.35f;
    mat.uvTiling = glm::vec2(4.0f, 4.0f);
    return mat;
}

Material MaterialManager::createConcrete() {
    Material mat;
    mat.name = "Concrete";
    mat.albedo = glm::vec3(0.58f, 0.58f, 0.58f);
    mat.metallic = 0.0f;
    mat.roughness = 0.92f;
    return mat;
}

Material MaterialManager::createBrick() {
    Material mat;
    mat.name = "Brick";
    mat.albedo = glm::vec3(0.62f, 0.28f, 0.22f);
    mat.metallic = 0.0f;
    mat.roughness = 0.85f;
    mat.uvTiling = glm::vec2(3.0f, 3.0f);
    return mat;
}

Material MaterialManager::createGrass() {
    Material mat;
    mat.name = "Grass";
    mat.albedo = glm::vec3(0.24f, 0.48f, 0.18f);
    mat.metallic = 0.0f;
    mat.roughness = 0.90f;
    return mat;
}

Material MaterialManager::createSmoothPlastic() {
    Material mat;
    mat.name = "SmoothPlastic";
    mat.albedo = glm::vec3(0.85f, 0.85f, 0.88f);
    mat.metallic = 0.0f;
    mat.roughness = 0.20f;
    return mat;
}

Material MaterialManager::createNeon() {
    Material mat;
    mat.name = "Neon";
    mat.albedo = glm::vec3(0.2f, 0.6f, 1.0f);
    mat.emissive = glm::vec3(0.2f, 0.6f, 1.0f);
    mat.emissiveIntensity = 4.0f;
    mat.metallic = 0.0f;
    mat.roughness = 0.1f;
    return mat;
}

void MaterialManager::init() {
    s_materials.clear();
    addMaterial(createAsphalt());
    addMaterial(createWood());
    addMaterial(createWoodPlanks());
    addMaterial(createGlass());
    addMaterial(createMirror());
    addMaterial(createMetal());
    addMaterial(createDiamondPlate());
    addMaterial(createConcrete());
    addMaterial(createBrick());
    addMaterial(createGrass());
    addMaterial(createSmoothPlastic());
    addMaterial(createNeon());
}

void MaterialManager::addMaterial(const Material& mat) {
    s_materials[mat.name] = mat;
}

Material* MaterialManager::getMaterial(const std::string& name) {
    auto it = s_materials.find(name);
    if (it != s_materials.end()) {
        return &it->second;
    }
    return nullptr;
}

const std::unordered_map<std::string, Material>& MaterialManager::getAllMaterials() {
    return s_materials;
}

std::vector<std::string> MaterialManager::getMaterialNames() {
    std::vector<std::string> names;
    for (const auto& [name, _] : s_materials) {
        names.push_back(name);
    }
    return names;
}

} // namespace Djusov
