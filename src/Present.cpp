#include <limits>

#include "Present.h"
#include "util.h"

#undef max

void updateMVP(Vulkan& vk, void* data, size_t length) {
    auto dst = mapMemory(vk.device, vk.mvp.handle, vk.mvp.memory);
        memcpy(dst, data, length);
    unMapMemory(vk.device, vk.mvp.memory);
}

void present(Vulkan& vk, vector<vector<VkCommandBuffer>>& cmdss) {
    uint32_t imageIndex = 0;
    auto result = vkAcquireNextImageKHR(
        vk.device,
        vk.swap.handle,
        std::numeric_limits<uint64_t>::max(),
        vk.swap.imageReady,
        VK_NULL_HANDLE,
        &imageIndex
    );
    if ((result == VK_SUBOPTIMAL_KHR) ||
            (result == VK_ERROR_OUT_OF_DATE_KHR)) {
        // TODO(jan): implement resize
        throw std::runtime_error("could not acquire next image");
    } else if (result != VK_SUCCESS) {
        throw std::runtime_error("could not acquire next image");
    }

    vector<VkCommandBuffer> cmdBuffers;
    for (auto& cmds: cmdss) {
        cmdBuffers.push_back(cmds[imageIndex]);
    }

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = (uint32_t)cmdBuffers.size();
    submitInfo.pCommandBuffers = cmdBuffers.data();
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &vk.swap.imageReady;
    VkPipelineStageFlags waitStages[] = {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
    };
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &vk.swap.cmdBufferDone;
    vkQueueSubmit(vk.queue, 1, &submitInfo, nullptr);

    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &vk.swap.handle;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &vk.swap.cmdBufferDone;
    presentInfo.pImageIndices = &imageIndex;
    checkSuccess(vkQueuePresentKHR(vk.queue, &presentInfo));

    vkDeviceWaitIdle(vk.device);
}
