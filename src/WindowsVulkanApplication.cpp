#include "WindowsVulkanApplication.h"

WindowsVulkanApplication::
WindowsVulkanApplication(HINSTANCE hinstance, HWND hwnd):
        ENABLED_EXTENSIONS({ "VK_KHR_surface", "VK_KHR_win32_surface" }),
        _hinstance(hinstance),
        _hwnd(hwnd),
        VulkanApplication() {
    _enabledExtensionCount = (uint32_t)ENABLED_EXTENSIONS.size();
    _extensionNames = new const char*[_enabledExtensionCount];
    for (unsigned i = 0; i < _enabledExtensionCount; i++) {
        _extensionNames[i] = ENABLED_EXTENSIONS[i].c_str();
    }

    initVulkanInstance();
    initSurface();
    initPhysicalDevice();
    initDeviceAndQueues();
}

WindowsVulkanApplication::
~WindowsVulkanApplication() {
    delete[] _extensionNames;

    vkDestroySurfaceKHR(_instance, _surface, nullptr);
}

uint32_t WindowsVulkanApplication::
getEnabledExtensionCount() {
    return _enabledExtensionCount;
}

const char** WindowsVulkanApplication::
getEnabledExtensions() {
    return _extensionNames;
}

void WindowsVulkanApplication::
initSurface() {
    VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = {};
    surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    surfaceCreateInfo.hinstance = _hinstance;
    surfaceCreateInfo.hwnd = _hwnd;

    checkSuccess(
        vkCreateWin32SurfaceKHR(_instance, &surfaceCreateInfo, nullptr, &_surface),
        "could not create win32 surface"
    );
}
