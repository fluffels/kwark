#pragma once

#include <dinput.h>

#include <glm/vec2.hpp>

using glm::ivec2;

struct Mouse {
    LPDIRECTINPUTDEVICE8 device;

    Mouse(HINSTANCE);
    ivec2 getDelta();
};
