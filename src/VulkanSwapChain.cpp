#include "util.h"

#include "Vulkan.h"
#include "VulkanSynch.h"

void findSwapFormats(Vulkan& vk) {
    checkSuccess(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
        vk.gpu,
        vk.swap.surface,
        &vk.swap.surfaceCapabilities
    ));

    // TODO(jan): More safely pick format &c here.
    if (!(vk.swap.surfaceCapabilities.supportedUsageFlags &
          VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)) {
        throw runtime_error("surface does not support color attachment");
    }
    if (!(vk.swap.surfaceCapabilities.supportedCompositeAlpha &
          VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR)) {
        throw runtime_error("surface does not support opaque compositing");
    }

    uint32_t surfaceFormatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(
        vk.gpu,
        vk.swap.surface,
        &surfaceFormatCount,
        nullptr
    );
    vector<VkSurfaceFormatKHR> surfaceFormats(surfaceFormatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(
        vk.gpu,
        vk.swap.surface,
        &surfaceFormatCount,
        surfaceFormats.data()
    );

    vk.swap.format = surfaceFormats[0].format;
    vk.swap.colorSpace = surfaceFormats[0].colorSpace;
    for (auto format: surfaceFormats) {
        if ((format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) &&
                (format.format == VK_FORMAT_B8G8R8A8_SRGB)) {
            vk.swap.format = format.format;
            vk.swap.colorSpace = format.colorSpace;
            break;
        }
    }

    uint32_t presentModeCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(
        vk.gpu,
        vk.swap.surface,
        &presentModeCount,
        nullptr
    );
    vector<VkPresentModeKHR> presentModes(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(
        vk.gpu,
        vk.swap.surface,
        &presentModeCount,
        presentModes.data()
    );
    vk.swap.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    for (auto availablePresentMode: presentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            vk.swap.presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
        }
    }

    vk.swap.extent = vk.swap.surfaceCapabilities.currentExtent;
}

void createSwapChain(Vulkan& vk) {
    VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = vk.swap.surface;
    createInfo.minImageCount = vk.swap.surfaceCapabilities.minImageCount;
    createInfo.imageExtent = vk.swap.surfaceCapabilities.currentExtent;
    createInfo.oldSwapchain = VK_NULL_HANDLE;
    createInfo.imageFormat = vk.swap.format;
    createInfo.imageColorSpace = vk.swap.colorSpace;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    createInfo.presentMode = vk.swap.presentMode;
    createInfo.preTransform = vk.swap.surfaceCapabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.clipped = VK_FALSE;

    checkSuccess(vkCreateSwapchainKHR(
        vk.device, &createInfo, nullptr, &vk.swap.handle
    ));
}

void getImages(Vulkan& vk) {
    uint32_t count = 0;
    vkGetSwapchainImagesKHR(vk.device, vk.swap.handle, &count, nullptr);
    vector<VkImage> handles(count);
    vk.swap.images.resize(count);
    checkSuccess(vkGetSwapchainImagesKHR(
        vk.device,
        vk.swap.handle,
        &count,
        handles.data()
    ));
    for (int i = 0; i < handles.size(); i++) {
        vk.swap.images[i].handle = handles[i];
    }
}

void createViews(Vulkan& vk) {
    for (auto& image: vk.swap.images) {
        VkImageViewCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = image.handle;
        createInfo.format = vk.swap.format;
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
        createInfo.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        checkSuccess(vkCreateImageView(
            vk.device,
            &createInfo,
            nullptr,
            &image.view
        ));
    }
}

void createFramebuffers(Vulkan& vk) {
    for (auto& image: vk.swap.images) {
        VkImageView imageViews[] = { image.view, vk.depth.view };
        VkFramebufferCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        createInfo.attachmentCount = 2;
        createInfo.pAttachments = imageViews;
        createInfo.renderPass = vk.renderPass;
        createInfo.height = vk.swap.extent.height;
        createInfo.width = vk.swap.extent.width;
        createInfo.layers = 1;
        auto& fb = vk.swap.framebuffers.emplace_back();
        checkSuccess(vkCreateFramebuffer(vk.device, &createInfo, nullptr, &fb));
    }
}

void createSemaphores(Vulkan& vk) {
    vk.swap.imageReady = createSemaphore(vk.device);
    vk.swap.cmdBufferDone[0] = createSemaphore(vk.device);
    vk.swap.cmdBufferDone[1] = createSemaphore(vk.device);
}

void initVKSwapChain(Vulkan& vk) {
    findSwapFormats(vk);
    createSwapChain(vk);
    getImages(vk);
    createViews(vk);
    createSemaphores(vk);
}
