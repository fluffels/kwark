#include "VulkanApplication.h"

VulkanApplication::
VulkanApplication() {
    vkEnumerateInstanceVersion(&_version);
    logVersion(_version);

    initVkInstance();
    initPhysicalDevice();
}

VulkanApplication::
~VulkanApplication() {

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

        for (auto queueFamilyProperties: queueFamilyPropertySets) {
            if (queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                _physicalDevice = physicalDevice;
                LOG(INFO) << "selected physical device "
                          << deviceProperties.deviceName;
                return;
            }
        }
    }
}

void VulkanApplication::
logVersion(uint32_t version) {
    LOG(INFO) << "Instance version: " << VK_VERSION_MAJOR(version) << "."
                                      << VK_VERSION_MINOR(version) << "."
                                      << VK_VERSION_PATCH(version);
}