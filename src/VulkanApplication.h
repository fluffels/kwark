#pragma once

#include <string>
#include <vector>

#include <vulkan.h>

#include "easylogging++.h"

#include "Platform.h"
#include "util.h"

using std::runtime_error;
using std::string;
using std::vector;

class VulkanApplication {
    public:
        VulkanApplication(const Platform&);
        virtual ~VulkanApplication();

        uint32_t getEnabledExtensionCount();
        const char** getEnabledExtensions();

        uint32_t getEnabledLayerCount();
        const char** getEnabledLayers();
    
    protected:
        vector<string> _enabledExtensions;
        vector<string> _enabledLayers;

        VkInstance _instance;
        VkPhysicalDevice _physicalDevice;
        uint32_t _gfxQueueIndex;
        VkQueue _gfxQueue;
        VkDevice _device;
        uint32_t _version;
        VkSurfaceKHR _surface;

        void checkSuccess(VkResult result, const string& errorMessage);
        void checkVersion(uint32_t version);

        void initVulkanInstance();
        void initPhysicalDevice();
        void initDeviceAndQueues();
};
