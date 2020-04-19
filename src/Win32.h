#pragma once

#include <stdexcept>
#include <string>
#include <vector>

#include <windows.h>

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_win32.h>

#include "Platform.h"

using std::runtime_error;
using std::string;
using std::vector;

class Win32: public Platform {
    public:
        Win32(HINSTANCE, HWND);

        VkSurfaceKHR getSurface(const VkInstance&) const;
        vector<string> getExtensions() const;
    
    private:
        HINSTANCE _instance;
        HWND _window;
};
