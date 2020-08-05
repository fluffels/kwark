#pragma once

#include "util.h"
#include "Vulkan.h"

struct VulkanPipeline {
    VkPipeline handle;
    VkPipelineLayout layout;
    VulkanShader vertexShader;
    VulkanShader fragmentShader;
    VkDescriptorSetLayout descriptorLayout;
    VkDescriptorPool descriptorPool;
    VkDescriptorSet descriptorSet;
    VkVertexInputBindingDescription inputBinding;
    vector<VkVertexInputAttributeDescription> inputAttributes;
    bool needsTexCoords;
    bool needsNormals;
    bool needsColor;
};

void initVKPipeline(Vulkan& vk, char* name, VulkanPipeline& pipeline);
