#pragma once

#include <vulkan/vulkan.h>

struct VulkanImage {
    VkImage handle;
    VkImageView view;
    VkDeviceMemory memory;
};

VulkanImage createVulkanDepthBuffer(
    VkDevice,
    VkPhysicalDeviceMemoryProperties&,
    VkExtent2D,
    uint32_t
);

VulkanImage createVulkanTexture(
    VkDevice,
    VkPhysicalDeviceMemoryProperties&,
    VkExtent2D,
    uint32_t
);

void destroyVulkanImage(
    VkDevice device,
    VulkanImage image
);
