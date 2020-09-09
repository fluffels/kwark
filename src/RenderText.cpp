#pragma warning(disable: 4267)

#include "stb/stb_easy_font.h"

#include "RenderText.h"

const float SIZE_X = 0;
const float SIZE_Y = 0;
const uint32_t VERTICES_PER_QUAD = 4;

static VulkanPipeline pipeline;
static VulkanMesh mesh;

void createIndexBuffer(
    Vulkan& vk,
    VulkanMesh& mesh
) {
    uint32_t data[6];
    auto count = sizeof(data) / sizeof(uint32_t);
    auto size = sizeof(uint32_t) * count;

    data[0] = 0;
    data[1] = 1;
    data[2] = 2;
    data[3] = 2;
    data[4] = 3;
    data[5] = 0;
    
    createIndexBuffer(
        vk.device, vk.memories, vk.queueFamily, (uint32_t)size, mesh.iBuff
    );

    void *dst = mapMemory(vk.device, mesh.iBuff.handle, mesh.iBuff.memory);
        memcpy(dst, data, size);
    unMapMemory(vk.device, mesh.iBuff.memory);
}

void createVertexBuffer(
    Vulkan& vk,
    uint8_t* buffer,
    uint32_t quadCount,
    VulkanPipeline& pipeline,
    VulkanMesh& mesh
) {
    auto count = quadCount * VERTICES_PER_QUAD;
    auto stride = pipeline.inputBinding.stride;
    auto size = count * stride;

    float* data = (float*)malloc(size);
    float* vertex = data;
    for (uint32_t idx = 0; idx < quadCount * VERTICES_PER_QUAD; idx++) {
        // NOTE(jan): vertex positions
        for (int i = 0; i < 3; i++) {
            *vertex++ = *((float*)buffer);
            // NOTE(jan): each coord is stored as a 32 bit float
            buffer += 4;
        }
        // NOTE(jan): vertex colours
        for (int i = 0; i < 4; i++) {
            *vertex++ = ((uint8_t)*buffer++) / 255.f;
        }
    }

    createVertexBuffer(
        vk.device, vk.memories, vk.queueFamily, size, mesh.vBuff
    );

    void *dst = mapMemory(vk.device, mesh.vBuff.handle, mesh.vBuff.memory);
        memcpy(dst, data, size);
    unMapMemory(vk.device, mesh.vBuff.memory);

    mesh.idxCount = count;

    free(data);
}

void
recordTextCommandBuffers(Vulkan& vk, vector<VkCommandBuffer>& cmds, char* text) {
    char buffer[99999];
    auto quadCount = stb_easy_font_print(
        SIZE_X, SIZE_Y, text, NULL, buffer, sizeof(buffer)
    );

    initVKPipeline(vk, "text", pipeline);

    createVertexBuffer(vk, (uint8_t*)buffer, quadCount, pipeline, mesh);
    createIndexBuffer(vk, mesh);

    uint32_t framebufferCount = vk.swap.images.size();
    cmds.resize(framebufferCount);
    createCommandBuffers(vk.device, vk.cmdPoolTransient, framebufferCount, cmds);
    VkDeviceSize offsets[] = {0};
    for (size_t swapIdx = 0; swapIdx < framebufferCount; swapIdx++) {
        auto& cmd = cmds[swapIdx];
        beginFrameCommandBuffer(cmd);

        VkRenderPassBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        beginInfo.clearValueCount = 0;
        beginInfo.pClearValues = nullptr;
        beginInfo.framebuffer = vk.swap.framebuffers[swapIdx];
        beginInfo.renderArea.extent = vk.swap.extent;
        beginInfo.renderArea.offset = {0, 0};
        beginInfo.renderPass = vk.renderPassNoClear;

        vkCmdBeginRenderPass(cmd, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.handle);
        vkCmdBindIndexBuffer(cmd, mesh.iBuff.handle, 0, VK_INDEX_TYPE_UINT32);
        vkCmdBindVertexBuffers(cmd, 0, 1, &mesh.vBuff.handle, offsets);
        for (size_t quadIdx = 0; quadIdx < quadCount; quadIdx++) {
            vkCmdDrawIndexed(cmd, 6, 1, 0, quadIdx * VERTICES_PER_QUAD, 0);
        }
        vkCmdEndRenderPass(cmd);

        checkSuccess(vkEndCommandBuffer(cmd));
    }
}

void resetTextCommandBuffers(Vulkan& vk, vector<VkCommandBuffer>& cmds) {
    vkFreeCommandBuffers(
        vk.device,
        vk.cmdPoolTransient,
        cmds.size(),
        cmds.data()
    );
    vkFreeMemory(vk.device, mesh.iBuff.memory, nullptr);
    vkDestroyBufferView(vk.device, mesh.iBuff.view, nullptr);
    vkDestroyBuffer(vk.device, mesh.iBuff.handle, nullptr);
    vkFreeMemory(vk.device, mesh.vBuff.memory, nullptr);
    vkDestroyBufferView(vk.device, mesh.vBuff.view, nullptr);
    vkDestroyBuffer(vk.device, mesh.vBuff.handle, nullptr);
}
