#include <Windows.h>

#include "easylogging++.h"
INITIALIZE_EASYLOGGINGPP

#include "WindowsVulkanApplication.h"

using std::exception;

const int WIDTH = 640;
const int HEIGHT = 480;

LRESULT
WindowProc(
    HWND    window,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam
) {
    switch (message) {
        case WM_DESTROY:
            PostQuitMessage(0);
        case WM_KEYDOWN:
            if (wParam == VK_ESCAPE) PostQuitMessage(0);
    }
    return DefWindowProc(window, message, wParam, lParam);
}

int
WinMain(
    HINSTANCE instance,
    HINSTANCE prevInstance,
    LPSTR commandLine,
    int showCommand
) {
    LOG(INFO) << "Starting...";
    
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

    try {
        WindowsVulkanApplication vk(instance, window);
    } catch (exception e) {
        LOG(ERROR) << e.what();
    }

    MSG msg;
    BOOL result = GetMessage(&msg, (HWND) NULL, 0, 0);
    while ((result != 0) && (result != -1)) { 
        TranslateMessage(&msg); 
        DispatchMessage(&msg); 
        result = GetMessage(&msg, (HWND) NULL, 0, 0);
    } 

    return 0; 
}
