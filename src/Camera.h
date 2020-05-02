#pragma once

#include <glm/vec3.hpp>
// #include <glm/vec4.hpp>
// #include <glm/mat4x4.hpp>
// #include <glm/gtc/matrix_transform.hpp>

using namespace glm;

struct Camera {
    vec3 eye;
    vec3 at;
    vec3 up;
};
