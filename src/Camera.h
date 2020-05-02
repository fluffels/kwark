#pragma once

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

using glm::mat4;
using glm::vec3;

struct Camera {
    vec3 eye;
    vec3 at;
    vec3 up;

    mat4 mvp;

    Camera(float, float, float, float, float, float, float, float, float);
};
