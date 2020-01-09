#include "VulkanApplication.h"

VulkanApplication::
VulkanApplication(const Platform& platform):
        _enabledExtensions({ VK_KHR_SURFACE_EXTENSION_NAME }),
        _enabledLayers({ "VK_LAYER_LUNARG_standard_validation" }) {
    auto platformRequiredExtensions = platform.getExtensions();
    _enabledExtensions.insert(
        _enabledExtensions.end(),
        platformRequiredExtensions.begin(),
        platformRequiredExtensions.end()
    );

    vkEnumerateInstanceVersion(&_version);
    checkVersion(_version);

    initVulkanInstance();
    _surface = platform.getSurface(_instance);
    initPhysicalDevice();
    initDeviceAndQueues();
    initSwapChain();
}

VulkanApplication::
~VulkanApplication() {
    vkDestroySurfaceKHR(_instance, _surface, nullptr);
    vkDestroyDevice(_device, nullptr);
    vkDestroyInstance(_instance, nullptr);
}

uint32_t VulkanApplication::
getEnabledLayerCount() {
    return (uint32_t)_enabledLayers.size();
}

const char** VulkanApplication::
getEnabledLayers() {
    return stringVectorToC(_enabledLayers);
}

uint32_t VulkanApplication::
getEnabledExtensionCount() {
    return (uint32_t)_enabledExtensions.size();
}

const char** VulkanApplication::
getEnabledExtensions() {
    return stringVectorToC(_enabledExtensions);
}

void VulkanApplication::
checkSuccess(VkResult result, const string& errorMessage) {
    if (result != VK_SUCCESS) {
        throw runtime_error(errorMessage);
    }
}

void VulkanApplication::
checkVersion(uint32_t version) {
    auto major = VK_VERSION_MAJOR(version);
    auto minor = VK_VERSION_MINOR(version);
    auto patch = VK_VERSION_PATCH(version);

    LOG(INFO) << "Instance version: " << major << "."
                                      << minor << "."
                                      << patch;
    
    if ((major < 1) || (minor < 1) || (patch < 126)) {
        throw runtime_error("you need at least Vulkan 1.1.126");
    }
}

void VulkanApplication::
initVulkanInstance() {
    VkApplicationInfo applicationCreateInfo = {};
    applicationCreateInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;

    VkInstanceCreateInfo instanceCreateInfo = {};
    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pApplicationInfo = &applicationCreateInfo;

    instanceCreateInfo.enabledLayerCount = getEnabledLayerCount();
    auto enabledLayerNames = getEnabledLayers();
    instanceCreateInfo.ppEnabledLayerNames = enabledLayerNames;

    instanceCreateInfo.enabledExtensionCount = getEnabledExtensionCount();
    auto enabledExtensionNames = getEnabledExtensions();
    instanceCreateInfo.ppEnabledExtensionNames = enabledExtensionNames;
    
    auto result = vkCreateInstance(&instanceCreateInfo, nullptr, &_instance);

    delete[] enabledLayerNames;
    delete[] enabledExtensionNames;

    checkSuccess(
        result,
        "could not create vk instance"
    );
    LOG(INFO) << "created instance";
}

void VulkanApplication::
initPhysicalDevice() {
    uint32_t physicalDeviceCount = 0;
    checkSuccess(
        vkEnumeratePhysicalDevices(_instance, &physicalDeviceCount, nullptr),
        "could not retrieve physical device count"
    );
    LOG(INFO) << physicalDeviceCount << " physical device(s)";

    vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
    checkSuccess(
        vkEnumeratePhysicalDevices(_instance, &physicalDeviceCount,
                                   physicalDevices.data()),
        "could not retrieve physical device list"
    );

    for (auto physicalDevice: physicalDevices) {
        bool hasGraphicsQueue = false;
        VkBool32 hasPresentQueue = false;

        VkPhysicalDeviceProperties deviceProperties = {};
        vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);

        uint32_t queueFamilyPropertyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(
            physicalDevice, &queueFamilyPropertyCount, nullptr
        );
        vector<VkQueueFamilyProperties> queueFamilyPropertySets(
            queueFamilyPropertyCount
        );
        vkGetPhysicalDeviceQueueFamilyProperties(
            physicalDevice, &queueFamilyPropertyCount,
            queueFamilyPropertySets.data()
        );

        for (uint32_t index = 0; index < queueFamilyPropertyCount; index++) {
            auto queueFamilyProperties = queueFamilyPropertySets[index];
            if (queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                hasGraphicsQueue = true;
                _gfxFamily = index;
            }
            vkGetPhysicalDeviceSurfaceSupportKHR(
                physicalDevice,
                index,
                _surface,
                &hasPresentQueue
            );
            if (hasPresentQueue) {
                _presentFamily = index;
            }
        }

        if (hasGraphicsQueue && hasPresentQueue) {
            _physicalDevice = physicalDevice;
            LOG(INFO) << "selected physical device "
                      << deviceProperties.deviceName;
            return;
        }
    }

    throw runtime_error("no suitable physical device found");
}

void VulkanApplication::
initDeviceAndQueues() {
    float queuePriority = 1.f;

    vector<VkDeviceQueueCreateInfo> queueCreateInfos;

    VkDeviceQueueCreateInfo gfxQueueCreateInfo = {};
    gfxQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    gfxQueueCreateInfo.queueCount = 1;
    gfxQueueCreateInfo.queueFamilyIndex = _gfxFamily;
    gfxQueueCreateInfo.pQueuePriorities = &queuePriority;
    queueCreateInfos.push_back(gfxQueueCreateInfo);

    VkDeviceQueueCreateInfo presentQueueCreateInfo = {};
    presentQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    presentQueueCreateInfo.queueCount = 1;
    presentQueueCreateInfo.queueFamilyIndex = _presentFamily;
    presentQueueCreateInfo.pQueuePriorities = &queuePriority;
    queueCreateInfos.push_back(presentQueueCreateInfo);

    VkDeviceCreateInfo deviceCreateInfo = {};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(
        queueCreateInfos.size()
    );
    deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();

    auto result = vkCreateDevice(
        _physicalDevice,
        &deviceCreateInfo,
        nullptr,
        &_device
    );

    checkSuccess(
        result,
        "could not create device"
    );
    LOG(INFO) << "created device";

    vkGetDeviceQueue(_device, _gfxFamily, 0, &_gfxQueue);
    LOG(INFO) << "retrieved gfx queue";
    vkGetDeviceQueue(_device, _presentFamily, 0, &_presentQueue);
    LOG(INFO) << "retrieved gfx queue";
}

void VulkanApplication::
initSwapChain() {
    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    auto result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
        _physicalDevice,
        _surface,
        &surfaceCapabilities
    );
    checkSuccess(result, "could not query surface capabilities");

    VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = _surface;
    createInfo.minImageCount = surfaceCapabilities.minImageCount;
    createInfo.imageExtent = surfaceCapabilities.currentExtent;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    result = vkCreateSwapchainKHR(
        _device,
        &createInfo,
        nullptr,
        &_swapChain
    );
    checkSuccess(result, "could not create swap chain");
}