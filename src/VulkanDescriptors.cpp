#include "Vulkan.h"

void updateUniformBuffer(
    VkDevice device,
    VkDescriptorSet descriptorSet,
    uint32_t binding,
    VkBuffer buffer
) {
    VkDescriptorBufferInfo info;
    info.buffer = buffer;
    info.offset = 0;
    info.range = VK_WHOLE_SIZE;

    VkWriteDescriptorSet write = {};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.descriptorCount = 1;
    write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    write.dstBinding = binding;
    write.dstSet = descriptorSet;
    write.pBufferInfo = &info;

    vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
}

void updateCombinedImageSampler(
    VkDevice device,
    VkDescriptorSet descriptorSet,
    uint32_t binding,
    VulkanSampler* samplers,
    uint32_t count
) {
    vector<VkDescriptorImageInfo> infos(count);
    for (int i = 0; i < count; i++) {
        auto& info = infos[i];
        auto& sampler = samplers[i];
        info.imageView = sampler.image.view;
        info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        info.sampler = sampler.handle;
    }

    VkWriteDescriptorSet write = {};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.descriptorCount = (uint32_t)infos.size();
    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.dstSet = descriptorSet;
    write.dstBinding = binding;
    write.pImageInfo = infos.data();

    vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
}

void updateUniformTexelBuffer(
    VkDevice device,
    VkDescriptorSet descriptorSet,
    uint32_t binding,
    VkBufferView view
) {
    VkWriteDescriptorSet write = {};

    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.descriptorCount = 1;
    write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
    write.dstSet = descriptorSet;
    write.dstBinding = binding;
    write.pTexelBufferView = &view;

    vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
}
