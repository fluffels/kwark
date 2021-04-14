#include "DirectInput.h"
#include "Logging.h"

BOOL deviceCallback(
    LPCDIDEVICEINSTANCE lpddi,
    LPVOID pvRef
) {
    DIDEVICEINSTANCE device = *lpddi;
    INFO("found a controller: %s", device.tszInstanceName);
    ((DirectInput*)pvRef)->controllerGUID = device.guidInstance;
    ((DirectInput*)pvRef)->controllerFound = true;
    return DIENUM_STOP;
}

DirectInput::DirectInput(HINSTANCE instance):
        controllerFound(false),
        controller(nullptr),
        di(nullptr),
        mouse(nullptr) {
    auto result = DirectInput8Create(
        instance,
        DIRECTINPUT_VERSION,
        IID_IDirectInput8A,
        (LPVOID*)&di,
        NULL
    );
    DI_CHECK(result, "could not get dinput");

    result = di->EnumDevices(
        DI8DEVCLASS_GAMECTRL,
        (LPDIENUMDEVICESCALLBACKA)deviceCallback,
        this,
        0
    );
    DI_CHECK(result, "could not enumerate devices");

    if (controllerFound) {
        controller = new Controller(di, controllerGUID);
    }
    mouse = new Mouse(di);
}

DirectInput::~DirectInput() {
    delete controller;
    delete mouse;
}
