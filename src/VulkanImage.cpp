#include "VulkanImage.h"

VulkanImage::VulkanImage(VkDevice device,
                         VkPhysicalDeviceMemoryProperties& memories,
                         VkExtent2D extent,
                         uint32_t queueFamily):
    device(device),
    memories(memories),
    extent(extent),
    queueFamily(queueFamily)
{
    createImage();
    allocate();
    createView();
}

VulkanImage::~VulkanImage() {
    vkDestroyImageView(device, view, nullptr);
    vkFreeMemory(device, memory, nullptr);
    vkDestroyImage(device, image, nullptr);
}

VkMemoryRequirements VulkanImage::getMemoryRequirements() {
    VkMemoryRequirements requirements = {};
    vkGetImageMemoryRequirements(
        device,
        image,
        &requirements
    );
    return requirements;
}  

void VulkanImage::allocate() {
    auto requirements = getMemoryRequirements();
    auto typeIndex = selectMemoryTypeIndex(memories, requirements, 0);

    VkMemoryAllocateInfo allocateInfo = {};
    allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocateInfo.allocationSize = requirements.size;
    allocateInfo.memoryTypeIndex = typeIndex;
    
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
}

void VulkanImage::createImage() {
    VkImageCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    createInfo.arrayLayers = 1;
    createInfo.extent = { extent.width, extent.height, 1 };
    createInfo.format = VK_FORMAT_D32_SFLOAT;
    createInfo.imageType = VK_IMAGE_TYPE_2D;
    createInfo.mipLevels = 1;
    createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    createInfo.queueFamilyIndexCount = 1;
    createInfo.pQueueFamilyIndices = &queueFamily;
    createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    createInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    createInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    auto result = vkCreateImage(device, &createInfo, nullptr, &image);
    if (result != VK_SUCCESS) {
        throw runtime_error("could not create image");
    }
}

void VulkanImage::createView() {
    VkImageViewCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    createInfo.format = VK_FORMAT_D32_SFLOAT;
    createInfo.image = image;
    createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    createInfo.subresourceRange.baseArrayLayer = 0;
    createInfo.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
    createInfo.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
    auto result = vkCreateImageView(device, &createInfo, nullptr, &view);
    if (result != VK_SUCCESS) {
        throw runtime_error("could not create image view");  
    }
}
