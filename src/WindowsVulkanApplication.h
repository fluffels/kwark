#pragma once

#include <windows.h>

#include <vulkan.h>
#include <vulkan_win32.h>

#include "VulkanApplication.h"

class WindowsVulkanApplication: private VulkanApplication {
    public:
        WindowsVulkanApplication(HINSTANCE, HWND);
    
    private:
        HINSTANCE _hinstance;
        HWND _hwnd;

        void initSurface();
};
