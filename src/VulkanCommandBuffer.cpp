#include <stdexcept>

#include "VulkanCommandBuffer.h"

using std::runtime_error;

VkCommandPool createCommandPool(
    VkDevice device,
    uint32_t family,
    bool transient
) {
    VkCommandPool result = {};

    VkCommandPoolCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    if (transient) {
        createInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    }
    createInfo.queueFamilyIndex = family;

    auto code = vkCreateCommandPool(device, &createInfo, nullptr, &result);
    if (code != VK_SUCCESS) {
        throw runtime_error("could not create command pool");
    }
    return result;
}

VkCommandBuffer allocateCommandBuffer(
    VkDevice device,
    VkCommandPool pool
) {
    VkCommandBuffer result = {};

    VkCommandBufferAllocateInfo allocateInfo = {};
    allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocateInfo.commandBufferCount = 1;
    allocateInfo.commandPool = pool;
    allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

    auto code = vkAllocateCommandBuffers(device, &allocateInfo, &result);
    if (code != VK_SUCCESS) {
        throw runtime_error("could not allocate command buffer");
    }
    return result;
}

void beginCommandBuffer(
    VkCommandBuffer buffer
) {
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    auto code = vkBeginCommandBuffer(buffer, &beginInfo);
    if (code != VK_SUCCESS) {
        throw runtime_error("could not begin command buffer");
    }
}

void endCommandBuffer(
    VkCommandBuffer buffer
) {
    auto code = vkEndCommandBuffer(buffer);
    if (code != VK_SUCCESS) {
        throw runtime_error("could not end command buffer");
    }
}
