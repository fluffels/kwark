#include <stdexcept>

#include "Vulkan.h"

using std::runtime_error;

VkDeviceMemory allocateVulkanImage(
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
    VkImageType type,
    VkExtent2D extent,
    uint32_t layerCount,
    uint32_t family,
    VkFormat format,
    VkImageUsageFlags usage,
    VkImageCreateFlags flags
) {
    VkImage result;

    VkImageCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    createInfo.flags = flags;
    createInfo.imageType = VK_IMAGE_TYPE_2D;
    createInfo.extent = { extent.width, extent.height, 1 };
    createInfo.mipLevels = 1;
    createInfo.arrayLayers = layerCount;
    createInfo.format = format;
    createInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    createInfo.queueFamilyIndexCount = 1;
    createInfo.pQueueFamilyIndices = &family;
    createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
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
    VkImageViewType type,
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
    createInfo.viewType = type;
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
    VkImageType type,
    VkImageViewType viewType,
    VkExtent2D extent,
    uint32_t layerCount,
    uint32_t family,
    VkFormat format,
    VkImageUsageFlags usage,
    VkImageAspectFlags aspectMask,
    bool hostVisible = false,
    VkImageCreateFlags imageCreateFlags = 0
) {
    VulkanImage result = {};

    result.handle = createImage(
        device,
        type,
        extent,
        layerCount,
        family,
        format,
        usage,
        imageCreateFlags
    );

    auto reqs = getMemoryRequirements(device, result.handle);
    auto flags =
        hostVisible? VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT : 0;
    auto memType = selectMemoryTypeIndex(memories, reqs, flags);
    result.memory = allocateVulkanImage(device, reqs.size, memType, result.handle);

    result.view = createView(
        device, result.handle, viewType, format, aspectMask
    );

    return result;
}

void destroyVulkanImage(VkDevice device, VulkanImage image) {
    vkDestroyImageView(device, image.view, nullptr);
    vkFreeMemory(device, image.memory, nullptr);
    vkDestroyImage(device, image.handle, nullptr);
}

void destroyVulkanSampler(VkDevice device, VulkanSampler sampler) {
    vkDestroySampler(device, sampler.handle, nullptr);
    destroyVulkanImage(device, sampler.image);
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
        VK_IMAGE_TYPE_2D,
        VK_IMAGE_VIEW_TYPE_2D,
        extent,
        1,
        family,
        VK_FORMAT_D32_SFLOAT,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        VK_IMAGE_ASPECT_DEPTH_BIT
    );
    return result;
}

VulkanSampler createVulkanSampler(
    VkDevice device,
    VkPhysicalDeviceMemoryProperties& memories,
    VkImageType type,
    VkImageViewType viewType,
    VkExtent2D extent,
    uint32_t layerCount,
    uint32_t family,
    VkImageCreateFlags imageCreateFlags = (VkImageCreateFlags)0
) {
    VulkanSampler result = {};

    result.image = createVulkanImage(
        device,
        memories,
        type,
        viewType,
        extent,
        layerCount,
        family,
        VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT,
        false,
        imageCreateFlags
    );

    VkSamplerCreateInfo createInfo = { VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
    createInfo.magFilter = VK_FILTER_NEAREST;
    createInfo.minFilter = VK_FILTER_NEAREST;
    createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    // TODO(jan): Enable anisotropic filtering.
    createInfo.anisotropyEnable = VK_FALSE;
    // createInfo.maxAnisotropy = ;
    createInfo.compareEnable = VK_FALSE;
    // createInfo.minLod = ;
    // createInfo.maxLod = ;
    // createInfo.mipLodBias = ;

    auto code = vkCreateSampler(device, &createInfo, nullptr, &result.handle);
    if (code != VK_SUCCESS) {
        throw runtime_error("could not create sampler");
    }

    return result;
}

VulkanSampler createVulkanSampler2D(
    VkDevice device,
    VkPhysicalDeviceMemoryProperties& memories,
    VkExtent2D extent,
    uint32_t family
) {
    return createVulkanSampler(
        device,
        memories,
        VK_IMAGE_TYPE_2D,
        VK_IMAGE_VIEW_TYPE_2D,
        extent,
        1,
        family
    );
}

VulkanSampler createVulkanSamplerCube(
    VkDevice device,
    VkPhysicalDeviceMemoryProperties& memories,
    VkExtent2D extent,
    uint32_t family
) {
    VulkanImage result = {};

    return createVulkanSampler(
        device,
        memories,
        VK_IMAGE_TYPE_2D,
        VK_IMAGE_VIEW_TYPE_CUBE,
        extent,
        6,
        family,
        VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT
    );
}

void uploadTexture(
    VkDevice device,
    VkPhysicalDeviceMemoryProperties& memories,
    VkQueue queue,
    uint32_t queueFamily,
    VkCommandPool cmdPoolTransient,
    uint32_t width,
    uint32_t height,
    void* data,
    uint32_t size,
    VulkanSampler& sampler
) {
    VkExtent2D extent = { width, height };

    VulkanBuffer staging;
    createStagingBuffer(
        device,
        memories,
        queueFamily,
        size,
        staging
    );

    void* dst = mapMemory(device, staging.handle, staging.memory);
        memcpy(dst, data, size);
    unMapMemory(device, staging.memory);

    sampler = createVulkanSampler2D(
        device, memories, extent, queueFamily
    );
    auto& image = sampler.image;

    auto cmd = allocateCommandBuffer(device, cmdPoolTransient);
    beginOneOffCommandBuffer(cmd);

    {
        VkImageMemoryBarrier barrier = {};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.image = image.handle;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        vkCmdPipelineBarrier(
            cmd,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier
        );
    }

    {
        VkBufferImageCopy region = {};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageSubresource.mipLevel = 0;
        region.imageOffset = { 0, 0, 0 };
        region.imageExtent = { extent.width, extent.height, 1 };

        vkCmdCopyBufferToImage(
            cmd,
            staging.handle,
            image.handle,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &region
        );
    }

    {
        VkImageMemoryBarrier barrier = {};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.image = image.handle;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(
            cmd,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier
        );
    }

    endCommandBuffer(cmd);

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;
    vkQueueSubmit(queue, 1, &submitInfo, nullptr);
}
