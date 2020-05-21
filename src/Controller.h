#pragma once

#include <vector>

#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>

#include "easylogging++.h"

using std::vector;

struct DeviceState {
    uint32_t x;
    uint32_t y;
    uint32_t rX;
    uint32_t rY;
};

struct ControllerState {
    float x;
    float y;
    float rX;
    float rY;
};

struct Controller {
    LPDIRECTINPUTDEVICE8 device;

    DWORD dataSize;
    vector<DIOBJECTDATAFORMAT> dataFormats;

    Controller(IDirectInput8*, GUID);
    ControllerState getState();
};
