#pragma once

#include <vulkan/vulkan.h>

struct VulkanImage {
    VkImage handle;
    VkImageView view;
    VkDeviceMemory memory;
};

struct VulkanSampler {
    VulkanImage image;
    VkSampler handle;
};

VulkanImage createVulkanDepthBuffer(
    VkDevice,
    VkPhysicalDeviceMemoryProperties&,
    VkExtent2D,
    uint32_t
);

VulkanSampler createVulkanSampler(
    VkDevice,
    VkPhysicalDeviceMemoryProperties&,
    VkExtent2D,
    uint32_t
);

void destroyVulkanImage(
    VkDevice device,
    VulkanImage image
);

void destroyVulkanSampler(
    VkDevice device,
    VulkanSampler sampler
);

