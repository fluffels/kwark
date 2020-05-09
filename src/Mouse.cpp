#include "Mouse.h"

#include <exception>

#define DI_CHECK(e, m) if (e != DI_OK) throw new std::exception(m)
#define DIRECTINPUT_VERSION 0x0800

Mouse::Mouse(HINSTANCE instance) {
    IDirectInput8* directInput;
    auto result = DirectInput8Create(
        instance,
        DIRECTINPUT_VERSION,
        IID_IDirectInput8A,
        (LPVOID*)&directInput,
        NULL
    );
    DI_CHECK(result, "could not get dinput");

    directInput->CreateDevice(GUID_SysMouse, &device, NULL);
    DI_CHECK(result, "could not create mouse");

    DIPROPDWORD properties;
    properties.diph.dwSize = sizeof(DIPROPDWORD);
    properties.diph.dwHeaderSize = sizeof(DIPROPHEADER);
    properties.diph.dwObj = 0;
    properties.diph.dwHow = DIPH_DEVICE;
    properties.dwData = DIPROPAXISMODE_REL;
    result = device->SetProperty(DIPROP_AXISMODE, &properties.diph);
    DI_CHECK(result, "could not set mouse properties");

    result = device->SetDataFormat(&c_dfDIMouse);
    DI_CHECK(result, "could not set mouse data format");

    result = device->Acquire();
    DI_CHECK(result, "could not acquire mouse");
}

ivec2 Mouse::getDelta() {
    DIMOUSESTATE state;
    device->GetDeviceState(sizeof(state), &state);
    return ivec2(state.lX, state.lY);
}
