#include "Camera.h"

#include <glm/gtc/matrix_transform.hpp>

using glm::lookAt;
using glm::perspective;

Camera::Camera(
    float ex, float ey, float ez,
    float ax, float ay, float az,
    float ux, float uy, float uz
): eye(ex, ey, ez),
   at(ax, ay, az),
   up(ux, uy, uz)
{
    auto view = lookAt(eye, at, up);
    auto proj = perspective(
        glm::radians(45.f),
        640.f / 480.f,
        -1.f,
        1.f
    );
    mvp = proj * view;
}
