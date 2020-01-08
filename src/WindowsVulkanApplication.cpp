#include "WindowsVulkanApplication.h"

WindowsVulkanApplication::
WindowsVulkanApplication(HINSTANCE hinstance, HWND hwnd):
        _hinstance(hinstance),
        _hwnd(hwnd),
        VulkanApplication() {
    _enabledExtensions.push_back("VK_KHR_win32_surface");

    initVulkanInstance();
    initSurface();
    initPhysicalDevice();
    initDeviceAndQueues();
}

WindowsVulkanApplication::
~WindowsVulkanApplication() {
    vkDestroySurfaceKHR(_instance, _surface, nullptr);
}

void WindowsVulkanApplication::
initSurface() {
    VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = {};
    surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    surfaceCreateInfo.hinstance = _hinstance;
    surfaceCreateInfo.hwnd = _hwnd;

    auto result = vkCreateWin32SurfaceKHR(
        _instance,
        &surfaceCreateInfo,
        nullptr,
        &_surface
    );

    checkSuccess(
        result,
        "could not create win32 surface"
    );
}
