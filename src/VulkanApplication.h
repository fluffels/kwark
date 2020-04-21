#pragma once

#include <string>
#include <vector>

#include <vulkan/vulkan.h>

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

        VkSurfaceCapabilitiesKHR getSurfaceCapabilities();

        void present();
    
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
        VkPresentModeKHR _presentMode;
        VkSurfaceCapabilitiesKHR _surfaceCapabilities;
        VkSwapchainKHR _swapChain;
        VkExtent2D _swapChainExtent;
        VkFormat _swapImageFormat;
        VkColorSpaceKHR _swapImageColorSpace;
        vector<VkImage> _swapImages;
        vector<VkImageView> _swapImageViews;
        VkCommandPool _graphicsCommandPool;
        VkCommandPool _presentCommandPool;
        VkRenderPass _renderPass;
        vector<VkFramebuffer> _framebuffers;
        VkPipeline _pipeline;
        VkBuffer _vertexBuffer;
        vector<VkCommandBuffer> _clearCommandBuffers;
        vector<VkCommandBuffer> _swapCommandBuffers;
        VkSemaphore _imageReady;
        VkSemaphore _presentReady;

        void checkSuccess(VkResult result, const string& errorMessage);
        void checkVersion(uint32_t version);

        void createVulkanInstance();
        void createDebugCallback();
        void createPhysicalDevice();
        void createDeviceAndQueues();
        void createSwapChain();
        void createRenderPass();
        void createFramebuffers();
        VkShaderModule createShaderModule(const string&);
        VkShaderModule createShaderModule(const vector<char>& code);
        void createPipeline(VkShaderModule&, VkShaderModule&);
        void createGraphicsCommandPool();
        void createPresentCommandPool();
        void createClearCommandBuffer();
        void createSwapCommandBuffers();
        void createSemaphores();

        void checkSurfaceCapabilities();

        void destroyFramebuffers();

        void getSwapImages();

        void loadVertexBuffer();

        void recordCommandBuffers();
        void resizeSwapChainIfNecessary();
};
