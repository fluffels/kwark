#pragma once

#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>

#include "easylogging++.h"

#include "Controller.h"
#include "Mouse.h"

#define DI_CHECK(e, m) if (e != DI_OK) throw new std::exception(m)

struct DirectInput {
    IDirectInput8* di;

    Controller* controller;
    GUID controllerGUID;
    bool controllerFound;

    Mouse* mouse;

    DirectInput(HINSTANCE);
    ~DirectInput();
};
