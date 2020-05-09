#include "Mouse.h"

#include <exception>

#define DI_CHECK(e, m) if (e != DI_OK) throw new std::exception(m)

Mouse::Mouse(IDirectInput8* directInput) {
    auto result = directInput->CreateDevice(GUID_SysMouse, &device, NULL);
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
