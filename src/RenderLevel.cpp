#pragma warning(disable: 4267)

#include "Mesh.h"
#include "Vulkan.h"
#include "VulkanImage.h"
#include "VulkanPipeline.h"

void renderLevel(
    Vulkan& vk,
    BSPParser& map,
    vector<VkCommandBuffer>& cmds
) {
    VulkanPipeline pipeline;
    initVKPipeline(
        vk,
        "default",
        pipeline
    );

    auto& textures = map.textures;
    auto textureCount = textures->textures.size();
    vector<VulkanSampler> samplers(textureCount);
    for (int idx = 0; idx < textures->textureHeaders.size(); idx++) {
        auto& header = textures->textureHeaders[idx];
        if ((header.width > 0) && (header.height > 0)) {
            auto texNum = textures->textureIDMap[idx];
            auto& texture = textures->textures[texNum];
            auto& sampler = samplers[texNum];
            uint32_t size = texture.size() * sizeof(uint8_t);
            uploadTexture(
                vk.device,
                vk.memories,
                vk.queue,
                vk.queueFamily,
                vk.cmdPoolTransient,
                header.width,
                header.height,
                texture.data(),
                size,
                sampler
            );
        }
    }

    Mesh mesh(map);
    VulkanMesh vulkanMesh;
    uploadMesh(
        vk.device,
        vk.memories,
        vk.queueFamily,
        mesh.vertices.data(),
        mesh.vertices.size()*sizeof(Vertex),
        vulkanMesh
    );
    vulkanMesh.vCount = mesh.vertices.size();

    VulkanBuffer lightMapBuffer;
    uploadTexelBuffer(
        vk.device,
        vk.memories,
        vk.queueFamily,
        mesh.lightMap.data(),
        mesh.lightMap.size() * 4,
        lightMapBuffer
    );

    updateUniformBuffer(
        vk.device,
        pipeline.descriptorSet,
        0,
        vk.mvp.handle
    );

    updateCombinedImageSampler(
        vk.device,
        pipeline.descriptorSet,
        1,
        samplers.data(),
        samplers.size()
    );

    updateUniformTexelBuffer(
        vk.device,
        pipeline.descriptorSet,
        2,
        lightMapBuffer.view
    );

    uint32_t framebufferCount = vk.swap.images.size();
    cmds.resize(framebufferCount);
    createCommandBuffers(vk.device, vk.cmdPool, framebufferCount, cmds);
    for (size_t swapIdx = 0; swapIdx < framebufferCount; swapIdx++) {
        auto& cmd = cmds[swapIdx];
        beginFrameCommandBuffer(cmd);

        VkClearValue colorClear;
        colorClear.color = {1.f, 1.f, 1.f, 1.f};
        VkClearValue depthClear;
        depthClear.depthStencil = { 1.f, 0 };
        VkClearValue clears[] = { colorClear, depthClear };

        VkRenderPassBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        beginInfo.clearValueCount = 2;
        beginInfo.pClearValues = clears;
        beginInfo.framebuffer = vk.swap.framebuffers[swapIdx];
        beginInfo.renderArea.extent = vk.swap.extent;
        beginInfo.renderArea.offset = {0, 0};
        beginInfo.renderPass = vk.renderPass;

        vkCmdBeginRenderPass(cmd, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(
            cmd,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipeline.handle
        );
        vkCmdBindDescriptorSets(
            cmd,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipeline.layout,
            0,
            1,
            &pipeline.descriptorSet,
            0,
            nullptr
        );
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(
            cmd,
            0, 1,
            &vulkanMesh.vBuff.handle,
            offsets
        );
        vkCmdDraw(
            cmd,
            vulkanMesh.vCount, 1,
            0, 0
        );

        vkCmdEndRenderPass(cmd);

        checkSuccess(vkEndCommandBuffer(cmd));
    }
}
