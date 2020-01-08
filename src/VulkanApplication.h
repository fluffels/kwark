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
        VkInstance _instance;
        VkPhysicalDevice _physicalDevice;
        uint32_t _gfxQueueIndex;
        VkDevice _device;
        uint32_t _version;

        void checkSuccess(VkResult result, const string& errorMessage);

        void initVkInstance();
        void initPhysicalDevice();
        void initDevice();

        void logVersion(uint32_t version);
};
