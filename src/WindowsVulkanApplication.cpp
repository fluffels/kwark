#include "WindowsVulkanApplication.h"

WindowsVulkanApplication::
WindowsVulkanApplication(HINSTANCE hinstance, HWND hwnd):
        VulkanApplication(),
        _hinstance(hinstance),
        _hwnd(hwnd) {
    initVulkanInstance();
    initSurface();
    initPhysicalDevice();
    initDeviceAndQueues();
}

void WindowsVulkanApplication::
initSurface() {
    VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = {};
    surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    surfaceCreateInfo.hinstance = _hinstance;
    surfaceCreateInfo.hwnd = _hwnd;
}
