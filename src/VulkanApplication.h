#pragma once

#include <string>
#include <vector>

#include <vulkan.h>

#include "easylogging++.h"

#include "FileSystem.h"
#include "Platform.h"
#include "util.h"
#include "Vertex.h"

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
        uint32_t _gfxFamily;
        VkQueue _gfxQueue;
        uint32_t _presentFamily;
        VkQueue _presentQueue;
        VkDevice _device;
        uint32_t _version;
        VkSurfaceKHR _surface;
        VkSwapchainKHR _swapChain;
        VkExtent2D _swapChainExtent;
        VkFormat _swapImageFormat;
        VkColorSpaceKHR _swapImageColorSpace;
        vector<VkImage> _swapImages;
        vector<VkImageView> _swapImageViews;
        VkCommandPool _presentCommandPool;
        VkRenderPass _renderPass;
        vector<VkFramebuffer> _framebuffers;
        VkPipeline _pipeline;

        void createVulkanInstance();
        void createPhysicalDevice();
        void createDeviceAndQueues();
        void createSwapChain();
        void createRenderPass();
        void createFramebuffers();
        VkShaderModule createShaderModule(const string&);
        VkShaderModule createShaderModule(const vector<char>& code);
        void createPipeline(VkShaderModule*, VkShaderModule*);

        void checkSuccess(VkResult result, const string& errorMessage);
        void checkVersion(uint32_t version);

        void present();
};
