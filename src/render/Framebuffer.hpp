#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>

namespace Djusov {

class HDRFramebuffer {
public:
    HDRFramebuffer();
    ~HDRFramebuffer();

    bool init(int width, int height);
    void resize(int width, int height);
    void bind() const;
    void unbind() const;

    GLuint getColorTexture() const { return m_colorTexture; }
    GLuint getBrightTexture() const { return m_brightTexture; }
    GLuint getDepthTexture() const { return m_depthTexture; }
    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }

private:
    void cleanup();

    GLuint m_fbo;
    GLuint m_colorTexture;
    GLuint m_brightTexture;
    GLuint m_depthTexture;
    int m_width;
    int m_height;
};

class ShadowMapFramebuffer {
public:
    ShadowMapFramebuffer();
    ~ShadowMapFramebuffer();

    bool init(int resolution = 2048);
    void bind() const;
    void unbind() const;

    GLuint getDepthTexture() const { return m_depthTexture; }
    int getResolution() const { return m_resolution; }

private:
    void cleanup();

    GLuint m_fbo;
    GLuint m_depthTexture;
    int m_resolution;
};

class PlanarReflectionFramebuffer {
public:
    PlanarReflectionFramebuffer();
    ~PlanarReflectionFramebuffer();

    bool init(int width = 1024, int height = 1024);
    void bind() const;
    void unbind() const;

    GLuint getColorTexture() const { return m_colorTexture; }
    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }

private:
    void cleanup();

    GLuint m_fbo;
    GLuint m_colorTexture;
    GLuint m_depthRbo;
    int m_width;
    int m_height;
};

class PingPongBlurFramebuffer {
public:
    PingPongBlurFramebuffer();
    ~PingPongBlurFramebuffer();

    bool init(int width, int height);
    void resize(int width, int height);

    GLuint getFbo(int index) const { return m_fbo[index % 2]; }
    GLuint getTexture(int index) const { return m_texture[index % 2]; }
    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }

private:
    void cleanup();

    GLuint m_fbo[2];
    GLuint m_texture[2];
    int m_width;
    int m_height;
};

} // namespace Djusov
