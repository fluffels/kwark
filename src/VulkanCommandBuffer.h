#pragma once

#include <vector>
#include <vulkan/vulkan.h>

using std::vector;

VkCommandPool createCommandPool(VkDevice, uint32_t, bool=false);
VkCommandBuffer allocateCommandBuffer(VkDevice, VkCommandPool);
void beginOneOffCommandBuffer(VkCommandBuffer buffer);
void beginFrameCommandBuffer(VkCommandBuffer buffer);
void endCommandBuffer(VkCommandBuffer);
void createCommandBuffers(
    VkDevice device,
    VkCommandPool pool,
    uint32_t count,
    vector<VkCommandBuffer>& buffers
);
