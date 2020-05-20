#include <iomanip>

#include <Windows.h>

#include "easylogging++.h"
INITIALIZE_EASYLOGGINGPP

#include "Camera.h"
#include "DirectInput.h"
#include "Mesh.h"
#include "Mouse.h"
#include "PAKParser.h"
#include "VulkanApplication.h"
#include "Win32.h"

using std::exception;
using std::setprecision;
using std::fixed;
using std::setw;

#define WIN32_CHECK(e, m) if (e != S_OK) throw new std::runtime_error(m)

const int WIDTH = 800;
const int HEIGHT = 800;

const float DELTA_MOVE_PER_S = 200.f;
const float DELTA_ROTATE_PER_S = 3.14f;
const float MOUSE_SENSITIVITY = 0.1f;
const float JOYSTICK_SENSITIVITY = 100;

VulkanApplication* vk;
bool keyboard[VK_OEM_CLEAR] = {};

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

int MainLoop(
    HINSTANCE instance,
    HINSTANCE prevInstance,
    LPSTR commandLine,
    int showCommand,
    PAKParser& parser
) {
    LOG(INFO) << "Starting...";

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
        "quark",
        WS_POPUP | WS_VISIBLE,
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

    SetWindowPos(
        window,
        HWND_TOP,
        0,
        0,
        GetSystemMetrics(SM_CXSCREEN),
        GetSystemMetrics(SM_CYSCREEN),
        SWP_FRAMECHANGED
    );

    int errorCode = 0;
    try {
        Win32 platform(instance, window);

        LARGE_INTEGER parseStart, parseEnd;
        int64_t parseDelta;
        QueryPerformanceCounter(&parseStart);
        BSPParser* map = parser.loadMap("e1m2");
        Mesh mesh(*map);
        QueryPerformanceCounter(&parseEnd);
        parseDelta = parseEnd.QuadPart - parseStart.QuadPart;
        float parseS = (float)parseDelta / counterFrequency.QuadPart;
        LOG(INFO) << "parsing BSP took " << setprecision(6) << parseS << "s";

        auto playerStart = map->findEntityByName("info_player_start");
        auto origin = playerStart.origin;
        Camera camera;
        camera.eye = { origin.x, origin.y, origin.z };
        camera.at = camera.eye;
        camera.at.x += 1;

        vk = new VulkanApplication(platform, &camera, mesh, map->atlas);
        auto angle = (float)-playerStart.angle;
        camera.rotateY(angle);

        delete map;
        map = nullptr;

        DirectInput directInput(instance);
        Controller* controller = directInput.controller;
        Mouse* mouse = directInput.mouse;

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
                }
                if (keyboard['S']) {
                    camera.back(deltaMove);
                }
                if (keyboard['A']) {
                    camera.left(deltaMove);
                }
                if (keyboard['D']) {
                    camera.right(deltaMove);
                }

                float deltaMouseRotate =
                    MOUSE_SENSITIVITY;
                auto mouseDelta = mouse->getDelta();

                camera.rotateY((float)mouseDelta.x * deltaMouseRotate);
                camera.rotateX((float)-mouseDelta.y * deltaMouseRotate);

                float deltaJoystickRotate =
                    DELTA_ROTATE_PER_S * s * JOYSTICK_SENSITIVITY;
                if (controller) {
                    auto state = controller->getState();

                    camera.rotateY(state.rX * deltaJoystickRotate);
                    camera.rotateX(-state.rY * deltaJoystickRotate);
                    camera.right(state.x * deltaMove);
                    camera.forward(-state.y * deltaMove);
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
    PAKParser parser(pathToPAK);

    return MainLoop(
        instance,
        prevInstance,
        commandLine,
        showCommand,
        parser
    );
}
