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
        virtual ~VulkanApplication();

        virtual uint32_t getEnabledExtensionCount() = 0;
        virtual const char** getEnabledExtensions() = 0;

        virtual uint32_t getEnabledLayerCount();
        virtual const char** getEnabledLayers();
    
    protected:
        vector<string> ENABLED_LAYERS;
        uint32_t _enabledLayerCount;
        const char** _layerNames;

        VkInstance _instance;
        VkPhysicalDevice _physicalDevice;
        uint32_t _gfxQueueIndex;
        VkQueue _gfxQueue;
        VkDevice _device;
        uint32_t _version;
        VkSurfaceKHR _surface;

        /**
         * The constructor is protected to prevent direct instantiation of this
         * class. Before the class can be fully initialized, _surface must be
         * set by the subclass corresponding to the correct OS.
         */
        VulkanApplication();

        void checkSuccess(VkResult result, const string& errorMessage);

        void initVulkanInstance();
        void initPhysicalDevice();
        void initDeviceAndQueues();

        void logVersion(uint32_t version);
};
