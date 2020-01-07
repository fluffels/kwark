#pragma once

#include <vulkan.h>

#include "easylogging++.h"

class VulkanApplication {
    public:
        VulkanApplication();
        ~VulkanApplication();
    
    private:
        uint32_t _versionApi;
        uint32_t _versionInstance;

        void initVkInstance();

        void logVersion(uint32_t version);
};
