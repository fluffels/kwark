#include "Camera.h"

#include <glm/gtc/matrix_transform.hpp>

using glm::cross;
using glm::lookAt;
using glm::normalize;
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

void Camera::left(float d) {
    right(-d);
}

void Camera::forward(float d) {
    auto forward = at - eye;
    auto delta = forward * d;
    eye += delta;
    at += delta;
}

void Camera::right(float d) {
    auto forward = at - eye;
    auto right = normalize(cross(forward, up));
    auto delta = right * d;
    eye += delta;
    at += delta;
}
