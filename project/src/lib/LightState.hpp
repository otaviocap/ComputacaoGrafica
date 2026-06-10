#pragma once

#include <glm/glm.hpp>

inline constexpr int MAX_LIGHTS = 8;

struct LightState {
    glm::vec3 position;
    float padding1 = 0.0f;

    glm::vec3 color;
    int enabled = 0;
};
