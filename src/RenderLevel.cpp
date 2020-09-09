#pragma warning(disable: 4267)

#include "RenderLevel.h"
#include "Mesh.h"

void renderLevel(
    Vulkan& vk,
    BSPParser& map,
    vector<VkCommandBuffer>& cmds
) {
    const int DEFAULT = 0;
    const int SKY = 1;
    const int FLUID = 2;
    vector<VulkanPipeline> pipelines(3);
    initVKPipeline(vk, "default", pipelines[DEFAULT]);
    initVKPipeline(vk, "sky", pipelines[SKY]);
    initVKPipeline(vk, "fluid", pipelines[FLUID]);

    auto& textures = *map.textures;
    vector<VulkanSampler> defaultSamplers;
    vector<VulkanSampler> skySamplers;
    vector<VulkanSampler> fluidSamplers;
    for (auto& texture: textures.textures) {
        auto& sampler = defaultSamplers.emplace_back();
        uint32_t size = texture.texels.size() * sizeof(uint8_t);
        uploadTexture(
            vk.device,
            vk.memories,
            vk.queue,
            vk.queueFamily,
            vk.cmdPoolTransient,
            texture.width,
            texture.height,
            texture.texels.data(),
            size,
            sampler
        );
    }
    updateCombinedImageSampler(
        vk.device,
        pipelines[DEFAULT].descriptorSet,
        1,
        defaultSamplers.data(),
        defaultSamplers.size()
    );
    for (auto& texture: textures.skyTextures) {
        auto& sampler = skySamplers.emplace_back();
        uint32_t size = texture.texels.size() * sizeof(uint8_t);
        uploadTexture(
            vk.device,
            vk.memories,
            vk.queue,
            vk.queueFamily,
            vk.cmdPoolTransient,
            texture.width,
            texture.height,
            texture.texels.data(),
            size,
            sampler
        );
    }
    updateCombinedImageSampler(
        vk.device,
        pipelines[SKY].descriptorSet,
        1,
        skySamplers.data(),
        skySamplers.size()
    );
    for (auto& texture: textures.fluidTextures) {
        auto& sampler = fluidSamplers.emplace_back();
        uint32_t size = texture.texels.size() * sizeof(uint8_t);
        uploadTexture(
            vk.device,
            vk.memories,
            vk.queue,
            vk.queueFamily,
            vk.cmdPoolTransient,
            texture.width,
            texture.height,
            texture.texels.data(),
            size,
            sampler
        );
    }
    updateCombinedImageSampler(
        vk.device,
        pipelines[FLUID].descriptorSet,
        1,
        fluidSamplers.data(),
        fluidSamplers.size()
    );

    Mesh mesh(map);
    VulkanMesh defaultMesh;
    uploadMesh(
        vk.device,
        vk.memories,
        vk.queueFamily,
        mesh.vertices.data(),
        mesh.vertices.size()*sizeof(Vertex),
        defaultMesh
    );
    defaultMesh.vCount = mesh.vertices.size();
    VulkanMesh skyMesh;
    uploadMesh(
        vk.device,
        vk.memories,
        vk.queueFamily,
        mesh.skyVertices.data(),
        mesh.skyVertices.size()*sizeof(Vertex),
        skyMesh
    );
    skyMesh.vCount = mesh.skyVertices.size();
    VulkanMesh fluidMesh;
    uploadMesh(
        vk.device,
        vk.memories,
        vk.queueFamily,
        mesh.fluidVertices.data(),
        mesh.fluidVertices.size()*sizeof(Vertex),
        fluidMesh
    );
    fluidMesh.vCount = mesh.fluidVertices.size();

    VulkanBuffer lightMapBuffer;
    uploadTexelBuffer(
        vk.device,
        vk.memories,
        vk.queueFamily,
        mesh.lightMap.data(),
        mesh.lightMap.size() * 4,
        lightMapBuffer
    );
    updateUniformTexelBuffer(
        vk.device,
        pipelines[DEFAULT].descriptorSet,
        2,
        lightMapBuffer.view
    );

    for (auto& pipeline: pipelines) {
        updateUniformBuffer(
            vk.device,
            pipeline.descriptorSet,
            0,
            vk.mvp.handle
        );
    }

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
            pipelines[DEFAULT].handle
        );
        vkCmdBindDescriptorSets(
            cmd,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipelines[DEFAULT].layout,
            0,
            1,
            &pipelines[DEFAULT].descriptorSet,
            0,
            nullptr
        );
        VkDeviceSize offsets[] = {0};

        vkCmdBindVertexBuffers(
            cmd,
            0, 1,
            &defaultMesh.vBuff.handle,
            offsets
        );
        vkCmdDraw(
            cmd,
            defaultMesh.vCount, 1,
            0, 0
        );

        vkCmdBindPipeline(
            cmd,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipelines[SKY].handle
        );
        vkCmdBindDescriptorSets(
            cmd,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipelines[SKY].layout,
            0,
            1,
            &pipelines[SKY].descriptorSet,
            0,
            nullptr
        );

        vkCmdBindVertexBuffers(
            cmd,
            0, 1,
            &skyMesh.vBuff.handle,
            offsets
        );
        vkCmdDraw(
            cmd,
            skyMesh.vCount, 1,
            0, 0
        );

        vkCmdBindPipeline(
            cmd,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipelines[FLUID].handle
        );
        vkCmdBindDescriptorSets(
            cmd,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipelines[FLUID].layout,
            0,
            1,
            &pipelines[FLUID].descriptorSet,
            0,
            nullptr
        );

        vkCmdBindVertexBuffers(
            cmd,
            0, 1,
            &fluidMesh.vBuff.handle,
            offsets
        );
        vkCmdDraw(
            cmd,
            fluidMesh.vCount, 1,
            0, 0
        );

        vkCmdEndRenderPass(cmd);

        checkSuccess(vkEndCommandBuffer(cmd));
    }
}
