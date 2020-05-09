#pragma once

#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>

#include <glm/vec2.hpp>

using glm::ivec2;

struct Mouse {
    LPDIRECTINPUTDEVICE8 device;

    Mouse(IDirectInput8*);
    ivec2 getDelta();
};
