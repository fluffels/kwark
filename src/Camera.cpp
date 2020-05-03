#include "Camera.h"

#include <glm/gtc/matrix_transform.hpp>

using glm::lookAt;
using glm::perspective;

mat4 Camera::get() const {
    auto view = lookAt(eye, at, up);
    auto proj = perspective(fov, ar, nearz, farz);
    auto mvp = proj * view;
    return mvp;
}

void Camera::setAR(uint32_t w, uint32_t h) {
    ar = (float)w / (float)h;
}

void Camera::setFOV(float f) {
    fov = glm::radians(f);
}

void Camera::back(float d) {
    forward(-d);
}

void Camera::forward(float d) {
    auto direction = at - eye;
    auto forward = d * direction;
    eye += forward;
    at += forward;
}
