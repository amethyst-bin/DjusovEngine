#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/euler_angles.hpp>

#include <cmath>
#include <algorithm>

namespace Djusov {
namespace Math {

constexpr float PI = 3.14159265358979323846f;
constexpr float DEG2RAD = PI / 180.0f;
constexpr float RAD2DEG = 180.0f / PI;

inline float clamp(float v, float min_v, float max_v) {
    return std::max(min_v, std::min(max_v, v));
}

inline float lerp(float a, float b, float t) {
    return a + t * (b - a);
}

inline glm::vec3 lerp(const glm::vec3& a, const glm::vec3& b, float t) {
    return a + t * (b - a);
}

inline float snapToGrid(float val, float snap) {
    if (snap <= 0.0001f) return val;
    return std::round(val / snap) * snap;
}

inline glm::vec3 snapToGrid(const glm::vec3& v, float snap) {
    if (snap <= 0.0001f) return v;
    return glm::vec3(snapToGrid(v.x, snap), snapToGrid(v.y, snap), snapToGrid(v.z, snap));
}

} // namespace Math
} // namespace Djusov
