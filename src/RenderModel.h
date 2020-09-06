#pragma once

#include <vector>

#include "PAKParser.h"
#include "Vulkan.h"

using std::vector;

void initModels(
    Vulkan& vk,
    PAKParser& pak,
    vector<Entity>& entities
);

void recordModelCommandBuffers(
    Vulkan& vk,
    float epoch,
    vector<VkCommandBuffer>& cmds
);
