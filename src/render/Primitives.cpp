#include "render/Primitives.hpp"
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

namespace Djusov {

static const float PI = 3.14159265358979323846f;

// Helper to append a box given min/max bounds and transform
static void appendBox(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices, 
                      const glm::vec3& minB, const glm::vec3& maxB, const glm::mat4& transform = glm::mat4(1.0f)) {
    unsigned int baseIndex = static_cast<unsigned int>(vertices.size());

    struct Face {
        glm::vec3 n;
        glm::vec3 t;
        glm::vec3 c[4];
    };

    Face faces[6] = {
        // Front
        { glm::vec3(0, 0, 1), glm::vec3(1, 0, 0), { glm::vec3(minB.x, minB.y, maxB.z), glm::vec3(maxB.x, minB.y, maxB.z), glm::vec3(maxB.x, maxB.y, maxB.z), glm::vec3(minB.x, maxB.y, maxB.z) } },
        // Back
        { glm::vec3(0, 0, -1), glm::vec3(-1, 0, 0), { glm::vec3(maxB.x, minB.y, minB.z), glm::vec3(minB.x, minB.y, minB.z), glm::vec3(minB.x, maxB.y, minB.z), glm::vec3(maxB.x, maxB.y, minB.z) } },
        // Left
        { glm::vec3(-1, 0, 0), glm::vec3(0, 0, 1), { glm::vec3(minB.x, minB.y, minB.z), glm::vec3(minB.x, minB.y, maxB.z), glm::vec3(minB.x, maxB.y, maxB.z), glm::vec3(minB.x, maxB.y, minB.z) } },
        // Right
        { glm::vec3(1, 0, 0), glm::vec3(0, 0, -1), { glm::vec3(maxB.x, minB.y, maxB.z), glm::vec3(maxB.x, minB.y, minB.z), glm::vec3(maxB.x, maxB.y, minB.z), glm::vec3(maxB.x, maxB.y, maxB.z) } },
        // Top
        { glm::vec3(0, 1, 0), glm::vec3(1, 0, 0), { glm::vec3(minB.x, maxB.y, maxB.z), glm::vec3(maxB.x, maxB.y, maxB.z), glm::vec3(maxB.x, maxB.y, minB.z), glm::vec3(minB.x, maxB.y, minB.z) } },
        // Bottom
        { glm::vec3(0, -1, 0), glm::vec3(1, 0, 0), { glm::vec3(minB.x, minB.y, minB.z), glm::vec3(maxB.x, minB.y, minB.z), glm::vec3(maxB.x, minB.y, maxB.z), glm::vec3(minB.x, minB.y, maxB.z) } }
    };

    glm::vec2 uvs[4] = { {0, 0}, {1, 0}, {1, 1}, {0, 1} };
    glm::mat3 normMat = glm::transpose(glm::inverse(glm::mat3(transform)));

    for (int f = 0; f < 6; ++f) {
        unsigned int faceBase = baseIndex + f * 4;
        glm::vec3 n = glm::normalize(normMat * faces[f].n);
        glm::vec3 t = glm::normalize(normMat * faces[f].t);

        for (int v = 0; v < 4; ++v) {
            Vertex vert;
            vert.position = glm::vec3(transform * glm::vec4(faces[f].c[v], 1.0f));
            vert.normal = n;
            vert.texCoords = uvs[v];
            vert.tangent = t;
            vertices.push_back(vert);
        }

        indices.push_back(faceBase + 0);
        indices.push_back(faceBase + 1);
        indices.push_back(faceBase + 2);
        indices.push_back(faceBase + 0);
        indices.push_back(faceBase + 2);
        indices.push_back(faceBase + 3);
    }
}

std::shared_ptr<Mesh> Primitives::createCube() {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    appendBox(vertices, indices, glm::vec3(-0.5f), glm::vec3(0.5f));
    return std::make_shared<Mesh>(vertices, indices);
}

std::shared_ptr<Mesh> Primitives::createPlane(float width, float length, float uvScale) {
    std::vector<Vertex> vertices(4);
    float hW = width * 0.5f;
    float hL = length * 0.5f;

    vertices[0] = { glm::vec3(-hW, 0.0f,  hL), glm::vec3(0, 1, 0), glm::vec2(0, uvScale),       glm::vec3(1, 0, 0) };
    vertices[1] = { glm::vec3( hW, 0.0f,  hL), glm::vec3(0, 1, 0), glm::vec2(uvScale, uvScale), glm::vec3(1, 0, 0) };
    vertices[2] = { glm::vec3( hW, 0.0f, -hL), glm::vec3(0, 1, 0), glm::vec2(uvScale, 0),       glm::vec3(1, 0, 0) };
    vertices[3] = { glm::vec3(-hW, 0.0f, -hL), glm::vec3(0, 1, 0), glm::vec2(0, 0),             glm::vec3(1, 0, 0) };

    std::vector<unsigned int> indices = { 0, 1, 2, 0, 2, 3 };
    return std::make_shared<Mesh>(vertices, indices);
}

std::shared_ptr<Mesh> Primitives::createRamp() {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    // Wedge: bottom is 1x1, rises from z = 0.5 (y = 0) to z = -0.5 (y = 1)
    glm::vec3 v0(-0.5f, 0.0f,  0.5f); // front-left
    glm::vec3 v1( 0.5f, 0.0f,  0.5f); // front-right
    glm::vec3 v2( 0.5f, 1.0f, -0.5f); // back-right-top
    glm::vec3 v3(-0.5f, 1.0f, -0.5f); // back-left-top
    glm::vec3 v4(-0.5f, 0.0f, -0.5f); // back-left-bot
    glm::vec3 v5( 0.5f, 0.0f, -0.5f); // back-right-bot

    auto addQuad = [&](const glm::vec3& a, const glm::vec3& b, const glm::vec3& c, const glm::vec3& d, const glm::vec3& n) {
        unsigned int base = static_cast<unsigned int>(vertices.size());
        glm::vec3 t(1, 0, 0);
        vertices.push_back({ a, n, {0, 0}, t });
        vertices.push_back({ b, n, {1, 0}, t });
        vertices.push_back({ c, n, {1, 1}, t });
        vertices.push_back({ d, n, {0, 1}, t });
        indices.push_back(base + 0); indices.push_back(base + 1); indices.push_back(base + 2);
        indices.push_back(base + 0); indices.push_back(base + 2); indices.push_back(base + 3);
    };

    auto addTri = [&](const glm::vec3& a, const glm::vec3& b, const glm::vec3& c, const glm::vec3& n) {
        unsigned int base = static_cast<unsigned int>(vertices.size());
        glm::vec3 t(1, 0, 0);
        vertices.push_back({ a, n, {0, 0}, t });
        vertices.push_back({ b, n, {1, 0}, t });
        vertices.push_back({ c, n, {0.5f, 1}, t });
        indices.push_back(base + 0); indices.push_back(base + 1); indices.push_back(base + 2);
    };

    // Sloped face (v0, v1, v2, v3)
    glm::vec3 slopeNormal = normalize(glm::cross(v1 - v0, v3 - v0));
    addQuad(v0, v1, v2, v3, slopeNormal);

    // Bottom face (v4, v5, v1, v0)
    addQuad(v4, v5, v1, v0, glm::vec3(0, -1, 0));

    // Back face (v5, v4, v3, v2)
    addQuad(v5, v4, v3, v2, glm::vec3(0, 0, -1));

    // Left triangle (v4, v0, v3)
    addTri(v4, v0, v3, glm::vec3(-1, 0, 0));

    // Right triangle (v1, v5, v2)
    addTri(v1, v5, v2, glm::vec3(1, 0, 0));

    return std::make_shared<Mesh>(vertices, indices);
}

std::shared_ptr<Mesh> Primitives::createSphere(int rings, int sectors) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    float const R = 1.0f / (float)(rings - 1);
    float const S = 1.0f / (float)(sectors - 1);

    for (int r = 0; r < rings; ++r) {
        for (int s = 0; s < sectors; ++s) {
            float y = sin(-PI * 0.5f + PI * r * R);
            float x = cos(2.0f * PI * s * S) * sin(PI * r * R);
            float z = sin(2.0f * PI * s * S) * sin(PI * r * R);

            glm::vec3 pos(x * 0.5f, y * 0.5f, z * 0.5f);
            glm::vec3 norm = normalize(glm::vec3(x, y, z));
            glm::vec2 uv(s * S, r * R);
            glm::vec3 tangent(-sin(2.0f * PI * s * S), 0, cos(2.0f * PI * s * S));

            vertices.push_back({ pos, norm, uv, tangent });
        }
    }

    for (int r = 0; r < rings - 1; ++r) {
        for (int s = 0; s < sectors - 1; ++s) {
            int i0 = r * sectors + s;
            int i1 = r * sectors + (s + 1);
            int i2 = (r + 1) * sectors + (s + 1);
            int i3 = (r + 1) * sectors + s;

            indices.push_back(i0); indices.push_back(i1); indices.push_back(i2);
            indices.push_back(i0); indices.push_back(i2); indices.push_back(i3);
        }
    }

    return std::make_shared<Mesh>(vertices, indices);
}

std::shared_ptr<Mesh> Primitives::createCylinder(int segments) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    float radius = 0.5f;
    float height = 1.0f;
    float halfH = height * 0.5f;

    // Body
    for (int i = 0; i <= segments; ++i) {
        float theta = (float)i / (float)segments * 2.0f * PI;
        float x = cos(theta) * radius;
        float z = sin(theta) * radius;
        glm::vec3 norm = normalize(glm::vec3(x, 0, z));
        float u = (float)i / (float)segments;

        vertices.push_back({ glm::vec3(x, -halfH, z), norm, {u, 0.0f}, glm::vec3(-z, 0, x) });
        vertices.push_back({ glm::vec3(x,  halfH, z), norm, {u, 1.0f}, glm::vec3(-z, 0, x) });
    }

    for (int i = 0; i < segments; ++i) {
        unsigned int b = i * 2;
        indices.push_back(b);
        indices.push_back(b + 1);
        indices.push_back(b + 3);

        indices.push_back(b);
        indices.push_back(b + 3);
        indices.push_back(b + 2);
    }

    // Top cap
    unsigned int topCenterIdx = static_cast<unsigned int>(vertices.size());
    vertices.push_back({ glm::vec3(0, halfH, 0), glm::vec3(0, 1, 0), {0.5f, 0.5f}, glm::vec3(1, 0, 0) });
    for (int i = 0; i <= segments; ++i) {
        float theta = (float)i / (float)segments * 2.0f * PI;
        float x = cos(theta) * radius;
        float z = sin(theta) * radius;
        vertices.push_back({ glm::vec3(x, halfH, z), glm::vec3(0, 1, 0), {0.5f + cos(theta)*0.5f, 0.5f + sin(theta)*0.5f}, glm::vec3(1, 0, 0) });
    }
    for (int i = 0; i < segments; ++i) {
        indices.push_back(topCenterIdx);
        indices.push_back(topCenterIdx + 1 + i);
        indices.push_back(topCenterIdx + 2 + i);
    }

    // Bottom cap
    unsigned int botCenterIdx = static_cast<unsigned int>(vertices.size());
    vertices.push_back({ glm::vec3(0, -halfH, 0), glm::vec3(0, -1, 0), {0.5f, 0.5f}, glm::vec3(1, 0, 0) });
    for (int i = 0; i <= segments; ++i) {
        float theta = (float)i / (float)segments * 2.0f * PI;
        float x = cos(theta) * radius;
        float z = sin(theta) * radius;
        vertices.push_back({ glm::vec3(x, -halfH, z), glm::vec3(0, -1, 0), {0.5f + cos(theta)*0.5f, 0.5f + sin(theta)*0.5f}, glm::vec3(1, 0, 0) });
    }
    for (int i = 0; i < segments; ++i) {
        indices.push_back(botCenterIdx);
        indices.push_back(botCenterIdx + 2 + i);
        indices.push_back(botCenterIdx + 1 + i);
    }

    return std::make_shared<Mesh>(vertices, indices);
}

std::shared_ptr<Mesh> Primitives::createScreenQuad() {
    std::vector<Vertex> vertices(4);
    vertices[0] = { glm::vec3(-1, -1, 0), glm::vec3(0, 0, 1), glm::vec2(0, 0), glm::vec3(1, 0, 0) };
    vertices[1] = { glm::vec3( 1, -1, 0), glm::vec3(0, 0, 1), glm::vec2(1, 0), glm::vec3(1, 0, 0) };
    vertices[2] = { glm::vec3( 1,  1, 0), glm::vec3(0, 0, 1), glm::vec2(1, 1), glm::vec3(1, 0, 0) };
    vertices[3] = { glm::vec3(-1,  1, 0), glm::vec3(0, 0, 1), glm::vec2(0, 1), glm::vec3(1, 0, 0) };

    std::vector<unsigned int> indices = { 0, 1, 2, 0, 2, 3 };
    return std::make_shared<Mesh>(vertices, indices);
}

std::shared_ptr<Mesh> Primitives::createWeaponMesh() {
    // Composite tactical rifle: receiver, barrel, magazine, sights
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    // 1. Main Receiver (central body)
    appendBox(vertices, indices, glm::vec3(-0.03f, -0.04f, -0.25f), glm::vec3(0.03f, 0.05f, 0.15f));

    // 2. Handguard & Barrel
    appendBox(vertices, indices, glm::vec3(-0.025f, -0.03f, -0.55f), glm::vec3(0.025f, 0.03f, -0.25f));
    appendBox(vertices, indices, glm::vec3(-0.012f, -0.012f, -0.72f), glm::vec3(0.012f, 0.012f, -0.55f));

    // 3. Muzzle Flash Hider
    appendBox(vertices, indices, glm::vec3(-0.016f, -0.016f, -0.76f), glm::vec3(0.016f, 0.016f, -0.72f));

    // 4. Pistol Grip
    glm::mat4 gripTransform = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -0.10f, 0.08f));
    gripTransform = glm::rotate(gripTransform, -0.25f, glm::vec3(1, 0, 0));
    appendBox(vertices, indices, glm::vec3(-0.02f, -0.07f, -0.025f), glm::vec3(0.02f, 0.06f, 0.025f), gripTransform);

    // 5. Curved Magazine
    glm::mat4 magTransform = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -0.12f, -0.08f));
    magTransform = glm::rotate(magTransform, 0.15f, glm::vec3(1, 0, 0));
    appendBox(vertices, indices, glm::vec3(-0.018f, -0.08f, -0.035f), glm::vec3(0.018f, 0.08f, 0.035f), magTransform);

    // 6. Stock (buttstock)
    appendBox(vertices, indices, glm::vec3(-0.025f, -0.05f, 0.15f), glm::vec3(0.025f, 0.04f, 0.40f));

    // 7. Top Picatinny Rail
    appendBox(vertices, indices, glm::vec3(-0.018f, 0.05f, -0.20f), glm::vec3(0.018f, 0.065f, 0.10f));

    // 8. Front Sight Post
    appendBox(vertices, indices, glm::vec3(-0.004f, 0.03f, -0.52f), glm::vec3(0.004f, 0.075f, -0.50f));
    // Front sight ring
    appendBox(vertices, indices, glm::vec3(-0.015f, 0.05f, -0.52f), glm::vec3(0.015f, 0.08f, -0.50f));

    // 9. Rear Aperture Sight (aligned for ADS aiming)
    appendBox(vertices, indices, glm::vec3(-0.02f, 0.065f, 0.05f), glm::vec3(-0.008f, 0.095f, 0.07f));
    appendBox(vertices, indices, glm::vec3(0.008f, 0.065f, 0.05f), glm::vec3(0.02f, 0.095f, 0.07f));
    appendBox(vertices, indices, glm::vec3(-0.02f, 0.090f, 0.05f), glm::vec3(0.02f, 0.10f, 0.07f));

    return std::make_shared<Mesh>(vertices, indices);
}

std::shared_ptr<Mesh> Primitives::createCharacterAvatarMesh() {
    // 3D humanoid avatar (head, torso, limbs) for 3rd-person & mirror reflections
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    // Torso: center around y = 1.0 (height 0.8, width 0.6, depth 0.35)
    appendBox(vertices, indices, glm::vec3(-0.3f, 0.6f, -0.175f), glm::vec3(0.3f, 1.4f, 0.175f));

    // Head: y = 1.4 to 1.85 (0.45 x 0.45 x 0.45)
    appendBox(vertices, indices, glm::vec3(-0.225f, 1.42f, -0.225f), glm::vec3(0.225f, 1.87f, 0.225f));

    // Left Arm: x = -0.55 to -0.3
    appendBox(vertices, indices, glm::vec3(-0.55f, 0.6f, -0.15f), glm::vec3(-0.32f, 1.38f, 0.15f));

    // Right Arm: x = 0.3 to 0.55
    appendBox(vertices, indices, glm::vec3(0.32f, 0.6f, -0.15f), glm::vec3(0.55f, 1.38f, 0.15f));

    // Left Leg: x = -0.28 to -0.04, y = 0.0 to 0.6
    appendBox(vertices, indices, glm::vec3(-0.28f, 0.0f, -0.16f), glm::vec3(-0.04f, 0.58f, 0.16f));

    // Right Leg: x = 0.04 to 0.28, y = 0.0 to 0.6
    appendBox(vertices, indices, glm::vec3(0.04f, 0.0f, -0.16f), glm::vec3(0.28f, 0.58f, 0.16f));

    return std::make_shared<Mesh>(vertices, indices);
}

} // namespace Djusov
