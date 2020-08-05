#pragma once

#include "Vulkan.h"

void present(Vulkan& vk, vector<vector<VkCommandBuffer>>& cmdss);
void updateMVP(Vulkan& vk, void* data, size_t length);

