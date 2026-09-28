#include "render/PBRRenderer.hpp"
#include <iostream>
#include <GL/glew.h>

namespace Djusov {

PBRRenderer::PBRRenderer()
    : m_width(1280), m_height(720), m_lightSpaceMatrix(1.0f) {}

PBRRenderer::~PBRRenderer() {}

bool PBRRenderer::init(int width, int height) {
    m_width = width;
    m_height = height;

    // Load shaders
    if (!m_pbrShader.loadFromFiles("assets/shaders/pbr.vert", "assets/shaders/pbr.frag")) {
        std::cerr << "[PBRRenderer] Failed to load PBR shader!" << std::endl;
        return false;
    }
    if (!m_skinnedShader.loadFromFiles("assets/shaders/skinned.vert", "assets/shaders/pbr.frag")) {
        std::cerr << "[PBRRenderer] Failed to load Skinned PBR shader!" << std::endl;
        return false;
    }
    if (!m_shadowShader.loadFromFiles("assets/shaders/shadow.vert", "assets/shaders/shadow.frag")) {
        std::cerr << "[PBRRenderer] Failed to load Shadow shader!" << std::endl;
        return false;
    }
    if (!m_skyShader.loadFromFiles("assets/shaders/sky.vert", "assets/shaders/sky.frag")) {
        std::cerr << "[PBRRenderer] Failed to load Sky shader!" << std::endl;
        return false;
    }
    if (!m_postProcessShader.loadFromFiles("assets/shaders/postprocess.vert", "assets/shaders/postprocess.frag")) {
        std::cerr << "[PBRRenderer] Failed to load PostProcess shader!" << std::endl;
        return false;
    }
    if (!m_blurShader.loadFromFiles("assets/shaders/blur.vert", "assets/shaders/blur.frag")) {
        std::cerr << "[PBRRenderer] Failed to load Blur shader!" << std::endl;
        return false;
    }

    // Initialize Framebuffers
    if (!m_hdrFbo.init(width, height)) return false;
    if (!m_viewportFbo.init(width, height)) return false;
    if (!m_shadowFbo.init(2048)) return false;
    if (!m_reflectionFbo.init(1024, 1024)) return false;
    if (!m_bloomFbo.init(width / 2, height / 2)) return false;

    // Geometry helpers
    m_screenQuad = Primitives::createScreenQuad();
    m_skyboxCube = Primitives::createCube();

    std::cout << "[PBRRenderer] Initialized successfully (" << width << "x" << height << ")" << std::endl;
    return true;
}

void PBRRenderer::resize(int width, int height) {
    if (width <= 0 || height <= 0) return;
    m_width = width;
    m_height = height;
    m_hdrFbo.resize(width, height);
    m_viewportFbo.resize(width, height);
    m_bloomFbo.resize(width / 2, height / 2);
}

void PBRRenderer::beginFrame() {
    m_hasReflection = false;
}

void PBRRenderer::renderShadowPass(const std::vector<RenderObject>& objects, const glm::vec3& sunDir, const glm::vec3& cameraPos) {
    glm::vec3 sunPos = cameraPos + normalize(sunDir) * 40.0f;
    glm::mat4 lightProjection = glm::ortho(-35.0f, 35.0f, -35.0f, 35.0f, 1.0f, 100.0f);
    glm::mat4 lightView = glm::lookAt(sunPos, cameraPos, glm::vec3(0.0f, 1.0f, 0.0f));
    m_lightSpaceMatrix = lightProjection * lightView;

    m_shadowFbo.bind();
    glClear(GL_DEPTH_BUFFER_BIT);
    glCullFace(GL_FRONT); // Peter-panning prevention

    m_shadowShader.use();
    m_shadowShader.setMat4("uLightSpaceMatrix", m_lightSpaceMatrix);

    for (const auto& obj : objects) {
        if (!obj.mesh || obj.material.isGlass) continue; // Glass doesn't cast harsh opaque shadows
        m_shadowShader.setMat4("uModel", obj.transform);
        obj.mesh->draw();
    }

    glCullFace(GL_BACK);
    m_shadowFbo.unbind();
}

void PBRRenderer::renderReflectionPass(const std::vector<RenderObject>& objects, const RenderObject* avatarObject,
                                      const glm::vec3& cameraPos, const glm::vec3& cameraForward,
                                      const glm::mat4& projection, const RenderObject& mirror) {
    m_hasReflection = true;

    // Compute reflected camera
    glm::vec3 N = normalize(mirror.mirrorNormal);
    glm::vec3 P0 = mirror.mirrorPoint;
    float dist = glm::dot(cameraPos - P0, N);
    glm::vec3 reflPos = cameraPos - 2.0f * dist * N;
    glm::vec3 reflForward = cameraForward - 2.0f * glm::dot(cameraForward, N) * N;
    glm::vec3 reflUp = glm::vec3(0, 1, 0) - 2.0f * glm::dot(glm::vec3(0, 1, 0), N) * N;
    if (glm::length(reflForward) > 0.001f) reflForward = glm::normalize(reflForward);
    else reflForward = glm::vec3(0, 0, -1);
    if (glm::length(reflUp) > 0.001f) reflUp = glm::normalize(reflUp);
    else reflUp = glm::vec3(0, 1, 0);
    if (std::abs(glm::dot(reflForward, reflUp)) > 0.99f) reflUp = glm::vec3(0, 0, 1);
    glm::mat4 reflView = glm::lookAt(reflPos, reflPos + reflForward, reflUp);

    m_reflectionFbo.bind();
    glClearColor(0.1f, 0.15f, 0.2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Render Sky in reflection
    glDepthFunc(GL_LEQUAL);
    m_skyShader.use();
    m_skyShader.setMat4("uView", reflView);
    m_skyShader.setMat4("uProjection", projection);
    m_skyShader.setVec3("uSunDir", glm::vec3(0.5f, 1.0f, 0.3f));
    m_skyShader.setVec3("uSunColor", glm::vec3(1.0f, 0.98f, 0.9f));
    m_skyboxCube->draw();
    glDepthFunc(GL_LESS);

    // Render world objects (excluding the mirror itself to avoid recursion)
    m_pbrShader.use();
    m_pbrShader.setMat4("uView", reflView);
    m_pbrShader.setMat4("uProjection", projection);
    m_pbrShader.setVec3("uCamPos", reflPos);
    m_pbrShader.setVec3("uSunDir", glm::vec3(0.5f, 1.0f, 0.3f));
    m_pbrShader.setVec3("uSunColor", glm::vec3(1.0f, 0.98f, 0.9f));
    m_pbrShader.setFloat("uSunIntensity", 2.5f);
    m_pbrShader.setMat4("uLightSpaceMatrix", m_lightSpaceMatrix);
    m_pbrShader.setInt("uNumPointLights", 0);

    for (const auto& obj : objects) {
        if (!obj.mesh || obj.isMirror) continue;
        m_pbrShader.setMat4("uModel", obj.transform);
        obj.material.apply(m_pbrShader);
        obj.mesh->draw();
    }

    // Render the 3D player avatar body in the mirror reflection!
    // (Notice: FPS ViewModel is culled, avatar is rendered!)
    if (avatarObject && avatarObject->mesh) {
        m_pbrShader.setMat4("uModel", avatarObject->transform);
        avatarObject->material.apply(m_pbrShader);
        avatarObject->mesh->draw();
    }

    m_reflectionFbo.unbind();
}

void PBRRenderer::renderMainPass(const std::vector<RenderObject>& objects,
                                const glm::mat4& view, const glm::mat4& projection,
                                const glm::vec3& cameraPos, const glm::vec3& sunDir, const glm::vec3& sunColor, float sunIntensity,
                                const std::vector<PointLightData>& pointLights) {
    m_hdrFbo.bind();
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // 1. Render Skybox
    glDepthFunc(GL_LEQUAL);
    m_skyShader.use();
    m_skyShader.setMat4("uView", view);
    m_skyShader.setMat4("uProjection", projection);
    m_skyShader.setVec3("uSunDir", sunDir);
    m_skyShader.setVec3("uSunColor", sunColor);
    m_skyboxCube->draw();
    glDepthFunc(GL_LESS);

    // 2. Setup PBR Shader
    m_pbrShader.use();
    m_pbrShader.setMat4("uView", view);
    m_pbrShader.setMat4("uProjection", projection);
    m_pbrShader.setVec3("uCamPos", cameraPos);
    m_pbrShader.setVec3("uSunDir", sunDir);
    m_pbrShader.setVec3("uSunColor", sunColor);
    m_pbrShader.setFloat("uSunIntensity", sunIntensity);
    m_pbrShader.setMat4("uLightSpaceMatrix", m_lightSpaceMatrix);

    // Shadow Map texture on slot 2
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, m_shadowFbo.getDepthTexture());
    m_pbrShader.setInt("uShadowMap", 2);

    // Reflection Map texture on slot 3
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, m_reflectionFbo.getColorTexture());
    m_pbrShader.setInt("uReflectionMap", 3);

    // Point lights
    int numLights = std::min(static_cast<int>(pointLights.size()), 16);
    m_pbrShader.setInt("uNumPointLights", numLights);
    for (int i = 0; i < numLights; ++i) {
        std::string base = "uPointLights[" + std::to_string(i) + "]";
        m_pbrShader.setVec3(base + ".position", pointLights[i].position);
        m_pbrShader.setVec3(base + ".color", pointLights[i].color);
        m_pbrShader.setFloat(base + ".intensity", pointLights[i].intensity);
        m_pbrShader.setFloat(base + ".radius", pointLights[i].radius);
    }

    // Render opaque and mirror objects
    for (const auto& obj : objects) {
        if (!obj.mesh || obj.material.isGlass) continue;
        m_pbrShader.setMat4("uModel", obj.transform);
        obj.material.apply(m_pbrShader);
        obj.mesh->draw();
    }

    // Render transparent/glass objects (with alpha blending)
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    for (const auto& obj : objects) {
        if (!obj.mesh || !obj.material.isGlass) continue;
        m_pbrShader.setMat4("uModel", obj.transform);
        obj.material.apply(m_pbrShader);
        obj.mesh->draw();
    }
    glDisable(GL_BLEND);

    m_hdrFbo.unbind();
}

void PBRRenderer::renderViewModel(const SkeletalMesh* handsMesh, const Mesh* weaponMesh,
                                 const glm::mat4& handsTransform, const glm::mat4& weaponTransform,
                                 const Material& handsMat, const Material& weaponMat,
                                 const glm::mat4& view, const glm::mat4& viewModelProjection,
                                 const glm::vec3& cameraPos, const glm::vec3& sunDir, const glm::vec3& sunColor) {
    m_hdrFbo.bind();
    glClear(GL_DEPTH_BUFFER_BIT); // Clear depth buffer so ViewModel never clips into walls!

    // 1. Skinned Hands
    if (handsMesh && handsMesh->isValid()) {
        m_skinnedShader.use();
        m_skinnedShader.setMat4("uView", view);
        m_skinnedShader.setMat4("uProjection", viewModelProjection);
        m_skinnedShader.setVec3("uCamPos", cameraPos);
        m_skinnedShader.setVec3("uSunDir", sunDir);
        m_skinnedShader.setVec3("uSunColor", sunColor);
        m_skinnedShader.setFloat("uSunIntensity", 3.0f);
        m_skinnedShader.setMat4("uLightSpaceMatrix", m_lightSpaceMatrix);
        m_skinnedShader.setInt("uNumPointLights", 0);
        m_skinnedShader.setMat4("uModel", handsTransform);

        handsMat.apply(m_skinnedShader);
        handsMesh->draw(m_skinnedShader);
    }

    // 2. Tactical Weapon
    if (weaponMesh) {
        m_pbrShader.use();
        m_pbrShader.setMat4("uView", view);
        m_pbrShader.setMat4("uProjection", viewModelProjection);
        m_pbrShader.setVec3("uCamPos", cameraPos);
        m_pbrShader.setVec3("uSunDir", sunDir);
        m_pbrShader.setVec3("uSunColor", sunColor);
        m_pbrShader.setFloat("uSunIntensity", 3.0f);
        m_pbrShader.setMat4("uLightSpaceMatrix", m_lightSpaceMatrix);
        m_pbrShader.setInt("uNumPointLights", 0);
        m_pbrShader.setMat4("uModel", weaponTransform);

        weaponMat.apply(m_pbrShader);
        weaponMesh->draw();
    }

    m_hdrFbo.unbind();
}

void PBRRenderer::renderPostProcess(float adsAmount, float exposure, GLuint targetFbo) {
    // 1. Gaussian Blur on bright buffer for Bloom
    bool horizontal = true, firstIteration = true;
    int amount = 6;
    m_blurShader.use();
    glViewport(0, 0, m_bloomFbo.getWidth(), m_bloomFbo.getHeight());

    for (int i = 0; i < amount; ++i) {
        glBindFramebuffer(GL_FRAMEBUFFER, m_bloomFbo.getFbo(horizontal));
        m_blurShader.setBool("uHorizontal", horizontal);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, firstIteration ? m_hdrFbo.getBrightTexture() : m_bloomFbo.getTexture(!horizontal));
        m_blurShader.setInt("uImage", 0);

        m_screenQuad->draw();
        horizontal = !horizontal;
        if (firstIteration) firstIteration = false;
    }

    // 2. Final Composite with ACES Tonemapping & ADS Scope Peripheral Blur
    glBindFramebuffer(GL_FRAMEBUFFER, targetFbo);
    glViewport(0, 0, m_width, m_height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_postProcessShader.use();

    // Scene HDR color on slot 0
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_hdrFbo.getColorTexture());
    m_postProcessShader.setInt("uSceneTexture", 0);

    // Bloom blurred texture on slot 1
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_bloomFbo.getTexture(!horizontal));
    m_postProcessShader.setInt("uBloomTexture", 1);

    m_postProcessShader.setBool("uBloomEnabled", true);
    m_postProcessShader.setFloat("uBloomIntensity", 0.35f);
    m_postProcessShader.setFloat("uADSAmount", adsAmount);
    m_postProcessShader.setFloat("uExposure", exposure);

    m_screenQuad->draw();
}

GLuint PBRRenderer::getFinalColorTexture() const {
    return m_hdrFbo.getColorTexture();
}

} // namespace Djusov
