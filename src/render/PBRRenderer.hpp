#pragma once

#include "render/Shader.hpp"
#include "render/Framebuffer.hpp"
#include "render/Mesh.hpp"
#include "render/Material.hpp"
#include "render/Primitives.hpp"
#include "render/SkeletalMesh.hpp"

#include <memory>
#include <vector>
#include <glm/glm.hpp>

namespace Djusov {

struct PointLightData {
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 color = glm::vec3(1.0f);
    float intensity = 5.0f;
    float radius = 15.0f;
};

struct RenderObject {
    std::shared_ptr<Mesh> mesh;
    glm::mat4 transform = glm::mat4(1.0f);
    Material material;
    bool isMirror = false;
    glm::vec3 mirrorNormal = glm::vec3(0.0f, 0.0f, 1.0f);
    glm::vec3 mirrorPoint = glm::vec3(0.0f);
};

class PBRRenderer {
public:
    PBRRenderer();
    ~PBRRenderer();

    bool init(int width, int height);
    void resize(int width, int height);

    void beginFrame();
    void renderShadowPass(const std::vector<RenderObject>& objects, const glm::vec3& sunDir, const glm::vec3& cameraPos);
    void renderReflectionPass(const std::vector<RenderObject>& objects, const RenderObject* avatarObject,
                             const glm::vec3& cameraPos, const glm::vec3& cameraForward,
                             const glm::mat4& projection, const RenderObject& mirror);
    void renderMainPass(const std::vector<RenderObject>& objects,
                        const glm::mat4& view, const glm::mat4& projection,
                        const glm::vec3& cameraPos, const glm::vec3& sunDir, const glm::vec3& sunColor, float sunIntensity,
                        const std::vector<PointLightData>& pointLights);
    void renderViewModel(const SkeletalMesh* handsMesh, const Mesh* weaponMesh,
                         const glm::mat4& handsTransform, const glm::mat4& weaponTransform,
                         const Material& handsMat, const Material& weaponMat,
                         const glm::mat4& view, const glm::mat4& viewModelProjection,
                         const glm::vec3& cameraPos, const glm::vec3& sunDir, const glm::vec3& sunColor);
    void renderPostProcess(float adsAmount, float exposure = 1.0f, GLuint targetFbo = 0);

    GLuint getFinalColorTexture() const;
    GLuint getViewportTexture() const { return m_viewportFbo.getColorTexture(); }
    GLuint getViewportFbo() const { return m_viewportFbo.getFbo(); }
    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }

    Shader& getPBRShader() { return m_pbrShader; }
    Shader& getSkinnedShader() { return m_skinnedShader; }

private:
    int m_width;
    int m_height;

    Shader m_pbrShader;
    Shader m_skinnedShader;
    Shader m_shadowShader;
    Shader m_skyShader;
    Shader m_postProcessShader;
    Shader m_blurShader;

    HDRFramebuffer m_hdrFbo;
    ViewportFramebuffer m_viewportFbo;
    ShadowMapFramebuffer m_shadowFbo;
    PlanarReflectionFramebuffer m_reflectionFbo;
    PingPongBlurFramebuffer m_bloomFbo;

    std::shared_ptr<Mesh> m_screenQuad;
    std::shared_ptr<Mesh> m_skyboxCube;

    glm::mat4 m_lightSpaceMatrix;
    bool m_hasReflection = false;
};

} // namespace Djusov
