#include "Win32.h"

Win32::
Win32(HINSTANCE instance, HWND window):
        _instance(instance),
        _window(window) {}

vector<string> Win32::
getExtensions() const {
    vector<string> result({ 
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME
    });
    return result;
}

VkSurfaceKHR Win32::
getSurface(const VkInstance& vkInstance) const {
    VkSurfaceKHR surface;

    VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = {};
    surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    surfaceCreateInfo.hinstance = _instance;
    surfaceCreateInfo.hwnd = _window;

    auto result = vkCreateWin32SurfaceKHR(
        vkInstance,
        &surfaceCreateInfo,
        nullptr,
        &surface
    );

    if (result != VK_SUCCESS) {
        throw runtime_error("could not create win32 surface");
    } else {
        return surface;
    }
}
