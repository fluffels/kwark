#include "Controller.h"

#define DI_CHECK(e, m) if (e != DI_OK) throw new std::exception(m)

const long JOYSTICK_MIN = 0;
const long JOYSTICK_MAX = 1000000;
const long JOYSTICK_RANGE = JOYSTICK_MAX - JOYSTICK_MIN;
const long JOYSTICK_MID = JOYSTICK_RANGE / 2;

BOOL objectCallback(
    LPCDIDEVICEOBJECTINSTANCE lpddoi,
    LPVOID pvRef
) {
    auto controller = (Controller*)pvRef;
    DIDEVICEOBJECTINSTANCE object = *lpddoi;
    LOG(INFO) << "found a controller object: " << object.tszName;
    DIOBJECTDATAFORMAT dataFormat = {};
    if (object.guidType == GUID_XAxis) {
        dataFormat.pguid = &GUID_XAxis;
        dataFormat.dwOfs = offsetof(DeviceState, x);
        controller->dataSize += sizeof(DeviceState::x);
    } else if (object.guidType == GUID_YAxis) {
        dataFormat.pguid = &GUID_YAxis;
        dataFormat.dwOfs = offsetof(DeviceState, y);
        controller->dataSize += sizeof(DeviceState::y);
    } else if (object.guidType == GUID_RxAxis) {
        dataFormat.pguid = &GUID_RxAxis;
        dataFormat.dwOfs = offsetof(DeviceState, rX);
        controller->dataSize += sizeof(DeviceState::rX);
    } else if (object.guidType == GUID_RyAxis) {
        dataFormat.pguid = &GUID_RyAxis;
        dataFormat.dwOfs = offsetof(DeviceState, rY);
        controller->dataSize += sizeof(DeviceState::rY);
    } else {
        return DIENUM_CONTINUE;
    }
    dataFormat.dwFlags = object.dwFlags;
    dataFormat.dwType = object.dwType;
    controller->dataFormats.push_back(dataFormat);
    return DIENUM_CONTINUE;
}

Controller::Controller(IDirectInput8* directInput, GUID guid):
        dataSize(0) {
    auto result = directInput->CreateDevice(guid, &device, NULL);
    DI_CHECK(result, "could not create controller");

    result = device->EnumObjects(
        objectCallback,
        this,
        0
    );
    DI_CHECK(result, "could not enumerate controller objects");

    DIDATAFORMAT dataFormat = {};
    dataFormat.dwSize = sizeof(dataFormat);
    dataFormat.dwObjSize = sizeof(DIOBJECTDATAFORMAT);
    dataFormat.dwFlags = DIDF_ABSAXIS;
    dataFormat.dwDataSize = dataSize;
    dataFormat.dwNumObjs = (DWORD)dataFormats.size();
    dataFormat.rgodf = dataFormats.data();

    result = device->SetDataFormat(&dataFormat);
    DI_CHECK(result, "could not set controller data format");

    DIPROPRANGE range;
    range.diph.dwSize = sizeof(DIPROPRANGE);
    range.diph.dwHeaderSize = sizeof(DIPROPHEADER);
    range.diph.dwObj = 0;
    range.diph.dwHow = DIPH_DEVICE;
    range.lMax = JOYSTICK_MAX;
    range.lMin = JOYSTICK_MIN;
    result = device->SetProperty(DIPROP_RANGE, &range.diph);
    DI_CHECK(result, "could not get controller properties");

    result = device->Acquire();
    DI_CHECK(result, "could not acquire controller");
}

ControllerState Controller::getState() {
    DeviceState deviceState;
    device->GetDeviceState(sizeof(deviceState), &deviceState);

    ControllerState state;
    state.rX = (deviceState.rX / (float)JOYSTICK_RANGE)*2 - 1;
    state.rY = (deviceState.rY / (float)JOYSTICK_RANGE)*2 - 1;
    state.x = (deviceState.x / (float)JOYSTICK_RANGE)*2 - 1;
    state.y = (deviceState.y / (float)JOYSTICK_RANGE)*2 - 1;
    return state;
}
