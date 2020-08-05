#include "util.h"
#include "VulkanBuffer.h"
#include "VulkanMemory.h"

void allocateVulkanBuffer(
    VkDevice device,
    VkPhysicalDeviceMemoryProperties memories,
    VulkanBuffer& buffer
) {
    auto requirements = getMemoryRequirements(device, buffer.handle);
    auto extraFlags = (
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    );
    auto typeIndex = selectMemoryTypeIndex(
        memories, requirements, extraFlags
    );

    VkMemoryAllocateInfo allocateInfo = {};
    allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocateInfo.allocationSize = requirements.size;
    allocateInfo.memoryTypeIndex = typeIndex;

    checkSuccess(vkAllocateMemory(
        device,
        &allocateInfo,
        nullptr,
        &buffer.memory
    ));

    checkSuccess(vkBindBufferMemory(
        device,
        buffer.handle,
        buffer.memory,
        0
    ));
}

void createVulkanBuffer(
    VkDevice device,
    uint32_t queueFamily,
    VkBufferUsageFlags usage,
    uint32_t size,
    VulkanBuffer& buffer
) {
    VkBufferCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.usage = usage;
    createInfo.queueFamilyIndexCount = 1;
    createInfo.pQueueFamilyIndices = &queueFamily;
    createInfo.size = size;

    checkSuccess(vkCreateBuffer(
        device,
        &createInfo,
        nullptr,
        &buffer.handle
    ));
}

void createVulkanBufferView(
    VkDevice device,
    VkFormat format,
    VulkanBuffer& buffer
) {
    VkBufferViewCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
    info.buffer = buffer.handle;
    info.format = format;
    info.range = VK_WHOLE_SIZE;
    checkSuccess(vkCreateBufferView(
        device,
        &info,
        nullptr,
        &buffer.view
    ));
}

void createUniformBuffer(
    VkDevice device,
    VkPhysicalDeviceMemoryProperties& memories,
    uint32_t queueFamily,
    uint32_t size,
    VulkanBuffer& buffer
) {
    auto usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    createVulkanBuffer(device, queueFamily, usage, size, buffer);
    allocateVulkanBuffer(device, memories, buffer);
}

void createVertexBuffer(
    VkDevice device,
    VkPhysicalDeviceMemoryProperties& memories,
    uint32_t queueFamily,
    uint32_t size,
    VulkanBuffer& buffer
) {
    auto usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    createVulkanBuffer(device, queueFamily, usage, size, buffer);
    allocateVulkanBuffer(device, memories, buffer);
}

void createIndexBuffer(
    VkDevice device,
    VkPhysicalDeviceMemoryProperties& memories,
    uint32_t queueFamily,
    uint32_t size,
    VulkanBuffer& buffer
) {
    auto usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    createVulkanBuffer(device, queueFamily, usage, size, buffer);
    allocateVulkanBuffer(device, memories, buffer);
}

void createStagingBuffer(
    VkDevice device,
    VkPhysicalDeviceMemoryProperties& memories,
    uint32_t queueFamily,
    uint32_t size,
    VulkanBuffer& buffer
) {
    auto usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    createVulkanBuffer(device, queueFamily, usage, size, buffer);
    allocateVulkanBuffer(device, memories, buffer);
}

void createTexelBuffer(
    VkDevice device,
    VkPhysicalDeviceMemoryProperties& memories,
    uint32_t queueFamily,
    uint32_t size,
    VulkanBuffer& buffer
) {
    createVulkanBuffer(
        device,
        queueFamily,
        VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT,
        size,
        buffer
    );
    allocateVulkanBuffer(device, memories, buffer);
    createVulkanBufferView(
        device,
        VK_FORMAT_R32_SFLOAT,
        buffer
    );
}

void uploadTexelBuffer(
    VkDevice device,
    VkPhysicalDeviceMemoryProperties& memories,
    uint32_t queueFamily,
    void* data,
    uint32_t size,
    VulkanBuffer& buffer
) {
    createTexelBuffer(device, memories, queueFamily, size, buffer);
    void* memory = mapMemory(device, buffer.handle, buffer.memory);
        memcpy(memory, data, size);
    unMapMemory(device, buffer.memory);
}
