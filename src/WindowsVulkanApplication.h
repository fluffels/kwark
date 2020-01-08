#pragma once

#include <windows.h>

#include <vulkan.h>
#include <vulkan_win32.h>

#include "VulkanApplication.h"

class WindowsVulkanApplication: private VulkanApplication {
    public:
        WindowsVulkanApplication(HINSTANCE, HWND);
        ~WindowsVulkanApplication();

        virtual uint32_t getEnabledExtensionCount();
        virtual const char** getEnabledExtensions();
    
    private:
        vector<string> ENABLED_EXTENSIONS;
        uint32_t _enabledExtensionCount;
        const char** _extensionNames;

        HINSTANCE _hinstance;
        HWND _hwnd;

        void initSurface();
};
