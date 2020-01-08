#include "VulkanApplication.h"

VulkanApplication::
VulkanApplication():
        ENABLED_LAYERS({ "VK_LAYER_LUNARG_standard_validation" }),
        ENABLED_EXTENSIONS({ "VK_KHR_surface" }) {
    vkEnumerateInstanceVersion(&_version);
    logVersion(_version);

    _enabledLayerCount = (uint32_t)ENABLED_LAYERS.size();
    _layerNames = new const char*[_enabledLayerCount];
    for (unsigned i = 0; i < _enabledLayerCount; i++) {
        _layerNames[i] = ENABLED_LAYERS[i].c_str();
    }

    _enabledExtensionCount = (uint32_t)ENABLED_EXTENSIONS.size();
    _extensionNames = new const char*[_enabledExtensionCount];
    for (unsigned i = 0; i < _enabledExtensionCount; i++) {
        _extensionNames[i] = ENABLED_EXTENSIONS[i].c_str();
    }

    initVkInstance();
    initPhysicalDevice();
    initDeviceAndQueues();
}

VulkanApplication::
~VulkanApplication() {
    delete[] _layerNames;
    delete[] _extensionNames;

    vkDestroyDevice(_device, nullptr);
    vkDestroyInstance(_instance, nullptr);
}

void VulkanApplication::
checkSuccess(VkResult result, const string& errorMessage) {
    if (result != VK_SUCCESS) {
        throw runtime_error(errorMessage);
    }
}

void VulkanApplication::
initVkInstance() {
    VkApplicationInfo applicationCreateInfo = {};
    applicationCreateInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;

    VkInstanceCreateInfo instanceCreateInfo = {};
    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pApplicationInfo = &applicationCreateInfo;
    instanceCreateInfo.enabledLayerCount = _enabledLayerCount;
    instanceCreateInfo.ppEnabledLayerNames = _layerNames;
    instanceCreateInfo.enabledExtensionCount = _enabledExtensionCount;
    instanceCreateInfo.ppEnabledExtensionNames = _extensionNames;

    checkSuccess(
        vkCreateInstance(&instanceCreateInfo, nullptr, &_instance),
        "could not create vk instance"
    );
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
                _physicalDevice = physicalDevice;
                _gfxQueueIndex = index;
                LOG(INFO) << "selected physical device "
                          << deviceProperties.deviceName;
                return;
            }
        }
    }

    throw runtime_error("no suitable physical device found");
}

void VulkanApplication::
initDeviceAndQueues() {
    float queuePriority = 1.f;

    VkDeviceQueueCreateInfo gfxQueueCreateInfo = {};
    gfxQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    gfxQueueCreateInfo.queueCount = 1;
    gfxQueueCreateInfo.queueFamilyIndex = _gfxQueueIndex;
    gfxQueueCreateInfo.pQueuePriorities = &queuePriority;

    vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    queueCreateInfos.push_back(gfxQueueCreateInfo);

    VkDeviceCreateInfo deviceCreateInfo = {};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(
        queueCreateInfos.size()
    );
    deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
    deviceCreateInfo.enabledLayerCount = _enabledLayerCount;
    deviceCreateInfo.ppEnabledLayerNames = _layerNames;

    checkSuccess(
        vkCreateDevice(_physicalDevice, &deviceCreateInfo, nullptr, &_device),
        "could not create device"
    );
    LOG(INFO) << "created device";

    vkGetDeviceQueue(_device, _gfxQueueIndex, 0, &_gfxQueue);
    LOG(INFO) << "retrieved gfx queue";
}

void VulkanApplication::
logVersion(uint32_t version) {
    LOG(INFO) << "Instance version: " << VK_VERSION_MAJOR(version) << "."
                                      << VK_VERSION_MINOR(version) << "."
                                      << VK_VERSION_PATCH(version);
}