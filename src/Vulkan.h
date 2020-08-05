#pragma once

#include <stdexcept>
#include <string>
#include <vector>

#include <vulkan/vulkan.h>

#include "SPIRV-Reflect/spirv_reflect.h"

#include "VulkanBuffer.h"
#include "VulkanCommandBuffer.h"
#include "VulkanImage.h"
#include "VulkanMemory.h"

using std::string;
using std::runtime_error;
using std::vector;

struct VulkanMesh {
    VulkanBuffer vBuff;
    uint32_t vCount;
    VulkanBuffer iBuff;
    uint32_t idxCount;
};

struct VulkanShader {
    VkShaderModule module;
    SpvReflectShaderModule reflect;
    vector<SpvReflectDescriptorSet*> sets;
};

struct VulkanSwapChain {
    VkPresentModeKHR presentMode;
    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    VkSwapchainKHR handle;
    VkExtent2D extent;
    VkFormat format;
    VkColorSpaceKHR colorSpace;
    vector<VulkanImage> images;
    vector<VkFramebuffer> framebuffers;
    VkSurfaceKHR surface;
    VkSemaphore imageReady;
// TODO(jan): handle multiple cmd buffers more flexibly
    VkSemaphore cmdBufferDone[2];
};

struct Vulkan {
    VkDebugReportCallbackEXT debugCallback;
    VkDevice device;
    VkInstance handle;
    VkPhysicalDevice gpu;
    vector<string> extensions;
    vector<string> layers;
    VkQueue queue;
    uint32_t queueFamily;
    VkPhysicalDeviceMemoryProperties memories;

// TODO(jan): more flexible handling of multiple render passes
    VkRenderPass renderPass;
    VkRenderPass renderPassNoClear;
    VulkanSwapChain swap;

    VulkanImage depth;
    VulkanBuffer mvp;

    VkCommandPool cmdPool;
    VkCommandPool cmdPoolTransient;
};

void createFramebuffers(Vulkan&);
void createVKInstance(Vulkan& vk);
void initVK(Vulkan& vk);
void initVKSwapChain(Vulkan& vk);

void uploadMesh(
    VkDevice device,
    VkPhysicalDeviceMemoryProperties& memories,
    uint32_t queueFamily,
    void* data,
    uint32_t size,
    VulkanMesh& mesh
);

void updateCombinedImageSampler(
    VkDevice device,
    VkDescriptorSet descriptorSet,
    uint32_t binding,
    VulkanSampler* samplers,
    uint32_t count
);
void updateUniformBuffer(
    VkDevice device,
    VkDescriptorSet descriptorSet,
    uint32_t binding,
    VkBuffer buffer
);
void updateUniformTexelBuffer(
    VkDevice device,
    VkDescriptorSet descriptorSet,
    uint32_t binding,
    VkBufferView view
);