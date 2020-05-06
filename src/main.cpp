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

const float DELTA_MOVE_PER_S = .5f;

VulkanApplication* vk;
bool keyboard[VK_OEM_CLEAR] = {};
GUID controllerGUID = {};

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
