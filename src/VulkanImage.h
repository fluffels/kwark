#pragma once

#include <stdexcept>

#include <vulkan/vulkan.h>

#include "VulkanUtils.h"

using std::runtime_error;

struct VulkanImage {
    VkImage image;
    VkImageView view;
    VkDeviceMemory memory;
    VkExtent2D extent;

    VulkanImage(VkDevice,
        VkPhysicalDeviceMemoryProperties&,
        VkExtent2D extent,
        uint32_t queueFamily);
    ~VulkanImage();

    void allocate();
    void createImage();
    void createView();
    VkMemoryRequirements getMemoryRequirements();

    private:
        VkDevice device;
        VkPhysicalDeviceMemoryProperties& memories;
        uint32_t queueFamily;
};
