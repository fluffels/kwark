#include <Windows.h>

#include "easylogging++.h"
INITIALIZE_EASYLOGGINGPP

#include "VulkanApplication.h"

using std::exception;

const int WIDTH = 640;
const int HEIGHT = 480;

LRESULT
WindowProc(
    HWND    hWnd,
    UINT    Msg,
    WPARAM  wParam,
    LPARAM  lParam
) {
    return 0;
}

int
WinMain(
    HINSTANCE instance,
    HINSTANCE prevInstance,
    LPSTR commandLine,
    int showCommand
) {
    LOG(INFO) << "Initializing...";
    try {
        VulkanApplication vk;
    } catch (exception e) {
        LOG(ERROR) << e.what();
    }
    
    WNDCLASSA windowClassProperties = {};
    windowClassProperties.style = CS_HREDRAW | CS_VREDRAW;
    windowClassProperties.lpfnWndProc = WindowProc;
    windowClassProperties.hInstance = instance;
    windowClassProperties.lpszClassName = "MainWindowClass";
    ATOM windowClass = RegisterClassA(&windowClassProperties);
    if (!windowClass) {
        LOG(ERROR) << "could not create window class";
    }

    auto window = CreateWindowA(
        "MainWindowClass",
        "studious-octo-enigma",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        NULL,
        NULL,
        instance,
        NULL
    );
    if (!window) {
        LOG(ERROR) << "could not create window";
    }
    return 0;
}
