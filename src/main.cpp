#include <Windows.h>
#include <dinput.h>

#include "easylogging++.h"
INITIALIZE_EASYLOGGINGPP

#include "Camera.h"
#include "PAKParser.h"
#include "VulkanApplication.h"
#include "Win32.h"

using std::exception;

#define WIN32_CHECK(e, m) if (e != S_OK) throw new std::runtime_error(m)

const int WIDTH = 640;
const int HEIGHT = 480;
const long JOYSTICK_MIN = 0;
const long JOYSTICK_MAX = 1000000;
const long JOYSTICK_RANGE = JOYSTICK_MAX - JOYSTICK_MIN;
const long JOYSTICK_MID = JOYSTICK_RANGE / 2;

const float DELTA_MOVE_PER_S = .5f;
const float DELTA_ROTATE_PER_S = 3.14 * 0.5f;

DIOBJECTDATAFORMAT xAxis = {};
DIOBJECTDATAFORMAT yAxis = {};
DIOBJECTDATAFORMAT rXAxis = {};
DIOBJECTDATAFORMAT rYAxis = {};

VulkanApplication* vk;
bool keyboard[VK_OEM_CLEAR] = {};
GUID controllerGUID = {};

struct ControllerState {
    uint32_t x;
    uint32_t y;
    uint32_t rX;
    uint32_t rY;
};

LPDIRECTINPUTDEVICE8
GetMouse(
    HINSTANCE instance
) {
    IDirectInput8* directInput;
    auto result = DirectInput8Create(
        instance,
        DIRECTINPUT_VERSION,
        IID_IDirectInput8A,
        (LPVOID*)&directInput,
        NULL
    );
    WIN32_CHECK(result, "could not get dinput");

    LPDIRECTINPUTDEVICE8 mouse;
    directInput->CreateDevice(GUID_SysMouse, &mouse, NULL);
    WIN32_CHECK(result, "could not create mouse");

    DIPROPDWORD properties;
    properties.diph.dwSize = sizeof(DIPROPDWORD);
    properties.diph.dwHeaderSize = sizeof(DIPROPHEADER);
    properties.diph.dwObj = 0;
    properties.diph.dwHow = DIPH_DEVICE;
    properties.dwData = DIPROPAXISMODE_REL;
    result = mouse->SetProperty(DIPROP_AXISMODE, &properties.diph);
    WIN32_CHECK(result, "could not set mouse properties");

    result = mouse->SetDataFormat(&c_dfDIMouse);
    WIN32_CHECK(result, "could not set mouse data format");

    result = mouse->Acquire();
    WIN32_CHECK(result, "could not acquire mouse");

    return mouse;
}

LRESULT
WindowProc(
    HWND    window,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam
) {
    switch (message) {
        case WM_SIZE:
            if (vk) vk->resize();
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        case WM_KEYDOWN:
            if (wParam == VK_ESCAPE) PostQuitMessage(0);
            else keyboard[(uint16_t)wParam] = true;
            break;
        case WM_KEYUP:
            keyboard[(uint16_t)wParam] = false;
            break;
    }
    return DefWindowProc(window, message, wParam, lParam);
}

BOOL DirectInputDeviceCallback(
    LPCDIDEVICEINSTANCE lpddi,
    LPVOID pvRef
) {
    DIDEVICEINSTANCE device = *lpddi;
    LOG(INFO) << "found a controller: " << device.tszInstanceName;
    controllerGUID = device.guidInstance;
    return DIENUM_STOP;
}

BOOL DirectInputControllerObjectsCallback(
    LPCDIDEVICEOBJECTINSTANCE lpddoi,
    LPVOID pvRef
) {
    DIDEVICEOBJECTINSTANCE object = *lpddoi;
    LOG(INFO) << "found a controller object: " << object.tszName;
    DIOBJECTDATAFORMAT *dataFormat = nullptr;
    if (object.guidType == GUID_XAxis) {
        dataFormat = &xAxis;
        dataFormat->pguid = &GUID_XAxis;
        dataFormat->dwOfs = 0;
    } else if (object.guidType == GUID_YAxis) {
        dataFormat = &yAxis;
        dataFormat->pguid = &GUID_YAxis;
        dataFormat->dwOfs = 4;
    } else if (object.guidType == GUID_RxAxis) {
        dataFormat = &rXAxis;
        dataFormat->pguid = &GUID_RxAxis;
        dataFormat->dwOfs = 8;
    } else if (object.guidType == GUID_RyAxis) {
        dataFormat = &rYAxis;
        dataFormat->pguid = &GUID_RyAxis;
        dataFormat->dwOfs = 12;
    }
    if (dataFormat != nullptr) {
        dataFormat->dwFlags = object.dwFlags;
        dataFormat->dwType = object.dwType;
    }
    return DIENUM_CONTINUE;
}

int MainLoop(
    HINSTANCE instance,
    HINSTANCE prevInstance,
    LPSTR commandLine,
    int showCommand
) {
    LOG(INFO) << "Starting...";

    LPDIRECTINPUTDEVICE8 mouse = GetMouse(instance);

    IDirectInput8* directInput;
    auto result = DirectInput8Create(
        instance,
        DIRECTINPUT_VERSION,
        IID_IDirectInput8A,
        (LPVOID*)&directInput,
        NULL
    );
    WIN32_CHECK(result, "could not get dinput");
    result = directInput->EnumDevices(
        DI8DEVCLASS_GAMECTRL,
        DirectInputDeviceCallback,
        0, 0
    );
    WIN32_CHECK(result, "could not enumerate devices");

    LPDIRECTINPUTDEVICE8 controller;
    if (controllerGUID.Data1 > 0) {
        result = directInput->CreateDevice(controllerGUID, &controller, NULL);
        WIN32_CHECK(result, "could not create controller");

        result = controller->EnumObjects(
            DirectInputControllerObjectsCallback,
            nullptr,
            0
        );
        WIN32_CHECK(result, "could not enumerate controller objects");

        DIOBJECTDATAFORMAT objectDataFormats[] = {
            xAxis,
            yAxis,
            rXAxis,
            rYAxis
        };

        DIDATAFORMAT dataFormat = {};
        dataFormat.dwSize = sizeof(dataFormat);
        dataFormat.dwObjSize = sizeof(DIOBJECTDATAFORMAT);
        dataFormat.dwFlags = DIDF_ABSAXIS;
        dataFormat.dwDataSize = 4 * 4;
        dataFormat.dwNumObjs = 4;
        dataFormat.rgodf = objectDataFormats;

        result = controller->SetDataFormat(&dataFormat);
        WIN32_CHECK(result, "could not set controller data format");

        DIPROPRANGE range;
        range.diph.dwSize = sizeof(DIPROPRANGE);
        range.diph.dwHeaderSize = sizeof(DIPROPHEADER);
        range.diph.dwObj = 0;
        range.diph.dwHow = DIPH_DEVICE;
        range.lMax = JOYSTICK_MAX;
        range.lMin = JOYSTICK_MIN;
        result = controller->SetProperty(DIPROP_RANGE, &range.diph);
        WIN32_CHECK(result, "could not get controller properties");
        LOG(INFO) << "controller range: " << range.lMin << " -> " << range.lMax;

        result = controller->Acquire();
        WIN32_CHECK(result, "could not acquire controller");
    }

    LARGE_INTEGER counterFrequency;
    QueryPerformanceFrequency(&counterFrequency);
    
    WNDCLASSEX windowClassProperties = {};
    windowClassProperties.cbSize = sizeof(windowClassProperties);
    windowClassProperties.style = CS_HREDRAW | CS_VREDRAW;
    windowClassProperties.lpfnWndProc = WindowProc;
    windowClassProperties.hInstance = instance;
    windowClassProperties.lpszClassName = "MainWindowClass";
    ATOM windowClass = RegisterClassEx(&windowClassProperties);
    if (!windowClass) {
        LOG(ERROR) << "could not create window class";
    }

    HWND window = CreateWindowEx(
        0,
        "MainWindowClass",
        "studious-octo-enigma",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        WIDTH,
        HEIGHT,
        NULL,
        NULL,
        instance,
        NULL
    );
    if (window == NULL) {
        LOG(ERROR) << "could not create window";
    }

    int errorCode = 0;
    try {
        Win32 platform(instance, window);
        Camera camera;
        vk = new VulkanApplication(platform, &camera);

        BOOL done = false;
        while (!done) {
            MSG msg;
            BOOL messageAvailable; 
            do {
                messageAvailable = PeekMessage(
                    &msg,
                    (HWND)nullptr,
                    0, 0,
                    PM_REMOVE
                );
                TranslateMessage(&msg); 
                if (msg.message == WM_QUIT) {
                    done = true;
                    errorCode = (int)msg.wParam;
                }
                DispatchMessage(&msg); 
            } while(!done && messageAvailable);

            if (!done) {
                LARGE_INTEGER frameStart, frameEnd;
                int64_t frameDelta;
                QueryPerformanceCounter(&frameStart);
                vk->present();
                QueryPerformanceCounter(&frameEnd);
                frameDelta = frameEnd.QuadPart - frameStart.QuadPart;
                float s = (float)frameDelta / counterFrequency.QuadPart;
                float fps = counterFrequency.QuadPart / (float)frameDelta;
                char buffer[255];
                sprintf_s(buffer, "%.2f FPS", fps);
                SetWindowText(window, buffer);

                float deltaMove = DELTA_MOVE_PER_S * s;
                if (keyboard['W']) {
                    camera.forward(deltaMove);
                } else if (keyboard['S']) {
                    camera.back(deltaMove);
                } else if (keyboard['A']) {
                    camera.left(deltaMove);
                } else if (keyboard['D']) {
                    camera.right(deltaMove);
                }

                DIMOUSESTATE mouseState;
                mouse->GetDeviceState(sizeof(mouseState), &mouseState);
                camera.rotateY((float)mouseState.lX);
                camera.rotateX((float)(-mouseState.lY));

                if (controllerGUID.Data1 > 0) {
                    ControllerState joyState;
                    controller->GetDeviceState(sizeof(joyState), &joyState);

                    float rX = (joyState.rX / (float)JOYSTICK_RANGE) - .5f;
                    rX *= DELTA_ROTATE_PER_S;
                    camera.rotateY(rX);

                    float rY = (joyState.rY / (float)JOYSTICK_RANGE) - .5f;
                    rY *= -DELTA_ROTATE_PER_S;
                    camera.rotateX(rY);
                }
            }
        } 
    } catch (exception e) {
        LOG(ERROR) << e.what();
    }

    delete vk;

    return errorCode; 
}

int
WinMain(
    HINSTANCE instance,
    HINSTANCE prevInstance,
    LPSTR commandLine,
    int showCommand
) {
    LPSTR pathToPAK = commandLine;
    LOG(INFO) << "path to PAK file: " << pathToPAK;
    parsePAK(pathToPAK);

    return MainLoop(instance, prevInstance, commandLine, showCommand);
}
