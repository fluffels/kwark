#include "Camera.h"

#include <glm/gtc/matrix_transform.hpp>

using glm::cross;
using glm::lookAt;
using glm::normalize;
using glm::perspective;
using glm::rotate;
using glm::vec4;

#define PI 3.14159265358979323846f

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

void Camera::rotateY(float d) {
    vec3 f = at - eye;
    vec4 forward = vec4(f, 0.0);
    mat4 rotation(1);
    rotation = rotate(rotation, PI * d * (1/180.f), up);
    forward = normalize(forward * rotation);
    at = eye + vec3(forward);
}

void Camera::rotateX(float d) {
    vec3 f = at - eye;
    vec4 forward = vec4(f, 0.0);
    mat4 rotation(1);
    auto right = normalize(cross(f, up));
    rotation = rotate(rotation, PI * d * (1/180.f), right);
    forward = normalize(forward * rotation);
    at = eye + vec3(forward);
}
