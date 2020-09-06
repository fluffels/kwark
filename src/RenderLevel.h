#pragma once

#include "BSPParser.h"
#include "Vulkan.h"

void renderLevel(
    Vulkan& vk,
    BSPParser& map,
    vector<VkCommandBuffer>& cmds
);
