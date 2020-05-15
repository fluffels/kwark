#include <stdexcept>

#include "VulkanImage.h"
#include "VulkanUtils.h"

using std::runtime_error;

VkDeviceMemory allocate(
    VkDevice device,
    VkDeviceSize size,
    uint32_t memoryType,
    VkImage image
) {
    VkMemoryAllocateInfo allocateInfo = {};
    allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocateInfo.allocationSize = size;
    allocateInfo.memoryTypeIndex = memoryType;
    
    VkDeviceMemory memory;
    auto result = vkAllocateMemory(
        device,
        &allocateInfo,
        nullptr,
        &memory
    );
    if (result != VK_SUCCESS) {
        throw runtime_error("could not allocate memory");
    }

    vkBindImageMemory(device, image, memory, 0);

    return memory;
}

VkImage createImage(
    VkDevice device,
    VkExtent2D extent,
    uint32_t family,
    VkFormat format,
    VkImageTiling tiling,
    VkImageUsageFlags usage
) {
    VkImage result;

    VkImageCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    createInfo.arrayLayers = 1;
    createInfo.extent = { extent.width, extent.height, 1 };
    createInfo.format = format;
    createInfo.imageType = VK_IMAGE_TYPE_2D;
    createInfo.mipLevels = 1;
    createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    createInfo.queueFamilyIndexCount = 1;
    createInfo.pQueueFamilyIndices = &family;
    createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.tiling = tiling;
    createInfo.usage = usage;
    createInfo.samples = VK_SAMPLE_COUNT_1_BIT;

    auto code = vkCreateImage(device, &createInfo, nullptr, &result);
    if (code != VK_SUCCESS) {
        throw runtime_error("could not create image");
    }

    return result;
}

VkImageView createView(
    VkDevice device,
    VkImage image,
    VkFormat format,
    VkImageAspectFlags aspectMask
) {
    VkImageView result;

    VkImageViewCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    createInfo.format = format;
    createInfo.image = image;
    createInfo.subresourceRange.aspectMask = aspectMask;
    createInfo.subresourceRange.baseArrayLayer = 0;
    createInfo.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
    createInfo.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;

    auto code = vkCreateImageView(device, &createInfo, nullptr, &result);
    if (code != VK_SUCCESS) {
        throw runtime_error("could not create image view");  
    }

    return result;
}

VulkanImage createVulkanImage(
    VkDevice device,
    VkPhysicalDeviceMemoryProperties& memories,
    VkExtent2D extent,
    uint32_t family,
    VkFormat format,
    VkImageUsageFlags usage,
    VkImageAspectFlags aspectMask,
    VkImageTiling tiling,
    bool hostVisible
) {
    VulkanImage result = {};

    result.handle = createImage(
        device,
        extent,
        family,
        format,
        tiling,
        usage
    );

    auto reqs = getMemoryRequirements(device, result.handle);
    auto flags =
        hostVisible? VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT : 0;
    auto memType = selectMemoryTypeIndex(memories, reqs, flags);
    result.memory = allocate(device, reqs.size, memType, result.handle);

    result.view = createView(device, result.handle, format, aspectMask);

    return result;
}

void destroyVulkanImage(VkDevice device, VulkanImage image) {
    vkDestroyImageView(device, image.view, nullptr);
    vkFreeMemory(device, image.memory, nullptr);
    vkDestroyImage(device, image.handle, nullptr);
}

VulkanImage createVulkanDepthBuffer(
    VkDevice device,
    VkPhysicalDeviceMemoryProperties& memories,
    VkExtent2D extent,
    uint32_t family
) {
    auto result = createVulkanImage(
        device,
        memories,
        extent,
        family,
        VK_FORMAT_D32_SFLOAT,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        VK_IMAGE_ASPECT_DEPTH_BIT,
        VK_IMAGE_TILING_OPTIMAL,
        false
    );
    return result;
}

VulkanImage createVulkanTexture(
    VkDevice device,
    VkPhysicalDeviceMemoryProperties& memories,
    VkExtent2D extent,
    uint32_t family
) {
    auto result = createVulkanImage(
        device,
        memories,
        extent,
        family,
        VK_FORMAT_R32G32B32_SFLOAT,
        VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT,
        VK_IMAGE_TILING_LINEAR,
        true
    );
    return result;
}
