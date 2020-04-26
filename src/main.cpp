#include <Windows.h>

#include "easylogging++.h"
INITIALIZE_EASYLOGGINGPP

#include "VulkanApplication.h"
#include "Win32.h"

using std::exception;

const int WIDTH = 640;
const int HEIGHT = 480;

VulkanApplication* vk;

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
            break;
    }
    return DefWindowProc(window, message, wParam, lParam);
}

int MainLoop(
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

    int errorCode = 0;
    try {
        Win32 platform(instance, window);
        vk = new VulkanApplication(platform);

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
                vk->present();
            }
        } 
    } catch (exception e) {
        LOG(ERROR) << e.what();
    }

    delete vk;

    return errorCode; 
}

struct PAKHeader {
    char id[4];
    int32_t offset;
    int32_t size;
};

struct FileEntry {
    char name[54];
    int32_t offset;
    int32_t size;
};

int
WinMain(
    HINSTANCE instance,
    HINSTANCE prevInstance,
    LPSTR commandLine,
    int showCommand
) {
    LPSTR pakName = commandLine;
    LOG(INFO) << "path to PAK file: " << commandLine;

    FILE* pakFile = nullptr;
    errno_t error = fopen_s(&pakFile, pakName, "r");
    if (error != 0) {
        LOG(ERROR) << "could not open PAK file";
        return 1;
    }

    PAKHeader header;
    fread_s(&header, sizeof(header), sizeof(header), 1, pakFile);
    if (strcmp("PACK", header.id) != 0) {
        LOG(ERROR) << "this is not a PAK file";
    }
    LOG(INFO) << "offset: " << header.offset;
    LOG(INFO) << "size: " << header.size;

    auto fileCount = header.size / 64;
    LOG(INFO) << "contains " << fileCount << " files";

    if (fseek(pakFile, header.offset, SEEK_SET) != 0) {
        LOG(ERROR) << "could not seek to file table";
        return 2;
    }

    for (int i = 0; i < fileCount; i++) {
        FileEntry fileEntry;
        fread_s(&fileEntry, sizeof(fileEntry), sizeof(fileEntry), 1, pakFile);
        LOG(INFO) << "file " << i << ": " << fileEntry.name;

        if (fseek(pakFile, header.offset + (64*i), SEEK_SET) != 0) {
            LOG(ERROR) << "could not seek to next file record";
            return 3;
        }
    }

    return MainLoop(instance, prevInstance, commandLine, showCommand);
}
