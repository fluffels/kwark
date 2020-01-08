#pragma once

#include <string>
#include <vector>

#include <vulkan.h>

#include "easylogging++.h"

using std::runtime_error;
using std::string;
using std::vector;

class VulkanApplication {
    public:
        VulkanApplication();
        ~VulkanApplication();
    
    private:
        vector<string> ENABLED_LAYERS;
        uint32_t _enabledLayerCount;
        const char** _layerNames;

        vector<string> ENABLED_EXTENSIONS;
        uint32_t _enabledExtensionCount;
        const char** _extensionNames;

        VkInstance _instance;
        VkPhysicalDevice _physicalDevice;
        uint32_t _gfxQueueIndex;
        VkQueue _gfxQueue;
        VkDevice _device;
        uint32_t _version;

        void checkSuccess(VkResult result, const string& errorMessage);

        void initVkInstance();
        void initPhysicalDevice();
        void initDeviceAndQueues();

        void logVersion(uint32_t version);
};
