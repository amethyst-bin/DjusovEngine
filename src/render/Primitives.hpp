#pragma once

#include "render/Mesh.hpp"
#include <memory>

namespace Djusov {

class Primitives {
public:
    static std::shared_ptr<Mesh> createCube();
    static std::shared_ptr<Mesh> createSphere(int rings = 20, int sectors = 20);
    static std::shared_ptr<Mesh> createCylinder(int segments = 20);
    static std::shared_ptr<Mesh> createPlane(float width = 50.0f, float length = 50.0f, float uvScale = 25.0f);
    static std::shared_ptr<Mesh> createRamp();
    static std::shared_ptr<Mesh> createScreenQuad();
    static std::shared_ptr<Mesh> createWeaponMesh();
    static std::shared_ptr<Mesh> createCharacterAvatarMesh();
};

} // namespace Djusov
