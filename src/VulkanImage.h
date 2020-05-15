#pragma once

#include <vulkan/vulkan.h>

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
    void mapMemory();
    void unMapMemory();

    private:
        VkDevice device;
        VkPhysicalDeviceMemoryProperties& memories;
        uint32_t queueFamily;
};