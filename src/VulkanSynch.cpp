#include "util.h"
#include "VulkanSynch.h"

VkSemaphore createSemaphore(VkDevice device) {
    VkSemaphore result;

    VkSemaphoreCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    checkSuccess(vkCreateSemaphore(
        device,
        &createInfo,
        nullptr,
        &result
    ));

    return result;
}