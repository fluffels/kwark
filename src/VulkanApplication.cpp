#include "VulkanApplication.h"

VulkanApplication::
VulkanApplication(const Platform& platform):
        _enabledExtensions({ VK_KHR_SURFACE_EXTENSION_NAME }),
        _enabledLayers({ "VK_LAYER_LUNARG_standard_validation" }) {
    auto platformRequiredExtensions = platform.getExtensions();
    _enabledExtensions.insert(
        _enabledExtensions.end(),
        platformRequiredExtensions.begin(),
        platformRequiredExtensions.end()
    );

    vkEnumerateInstanceVersion(&_version);
    checkVersion(_version);

    initVulkanInstance();
    _surface = platform.getSurface(_instance);
    initPhysicalDevice();
    initDeviceAndQueues();
    initSwapChain();
    // present();
}

VulkanApplication::
~VulkanApplication() {
    vkDestroySwapchainKHR(_device, _swapChain, nullptr);
    vkDestroySurfaceKHR(_instance, _surface, nullptr);
    vkDestroyDevice(_device, nullptr);
    vkDestroyInstance(_instance, nullptr);
}

uint32_t VulkanApplication::
getEnabledLayerCount() {
    return (uint32_t)_enabledLayers.size();
}

const char** VulkanApplication::
getEnabledLayers() {
    return stringVectorToC(_enabledLayers);
}

uint32_t VulkanApplication::
getEnabledExtensionCount() {
    return (uint32_t)_enabledExtensions.size();
}

const char** VulkanApplication::
getEnabledExtensions() {
    return stringVectorToC(_enabledExtensions);
}

void VulkanApplication::
checkSuccess(VkResult result, const string& errorMessage) {
    if (result != VK_SUCCESS) {
        throw runtime_error(errorMessage);
    }
}

void VulkanApplication::
checkVersion(uint32_t version) {
    auto major = VK_VERSION_MAJOR(version);
    auto minor = VK_VERSION_MINOR(version);
    auto patch = VK_VERSION_PATCH(version);

    LOG(INFO) << "Instance version: " << major << "."
                                      << minor << "."
                                      << patch;
    
    if ((major < 1) || (minor < 1) || (patch < 126)) {
        throw runtime_error("you need at least Vulkan 1.1.126");
    }
}

void VulkanApplication::
initVulkanInstance() {
    VkApplicationInfo applicationCreateInfo = {};
    applicationCreateInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;

    VkInstanceCreateInfo instanceCreateInfo = {};
    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pApplicationInfo = &applicationCreateInfo;

    instanceCreateInfo.enabledLayerCount = getEnabledLayerCount();
    auto enabledLayerNames = getEnabledLayers();
    instanceCreateInfo.ppEnabledLayerNames = enabledLayerNames;

    instanceCreateInfo.enabledExtensionCount = getEnabledExtensionCount();
    auto enabledExtensionNames = getEnabledExtensions();
    instanceCreateInfo.ppEnabledExtensionNames = enabledExtensionNames;
    
    auto result = vkCreateInstance(&instanceCreateInfo, nullptr, &_instance);

    delete[] enabledLayerNames;
    delete[] enabledExtensionNames;

    checkSuccess(
        result,
        "could not create vk instance"
    );
    LOG(INFO) << "created instance";
}

void VulkanApplication::
initPhysicalDevice() {
    uint32_t physicalDeviceCount = 0;
    checkSuccess(
        vkEnumeratePhysicalDevices(_instance, &physicalDeviceCount, nullptr),
        "could not retrieve physical device count"
    );
    LOG(INFO) << physicalDeviceCount << " physical device(s)";

    vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
    checkSuccess(
        vkEnumeratePhysicalDevices(_instance, &physicalDeviceCount,
                                   physicalDevices.data()),
        "could not retrieve physical device list"
    );

    for (auto physicalDevice: physicalDevices) {
        bool hasGraphicsQueue = false;
        VkBool32 hasPresentQueue = false;

        VkPhysicalDeviceProperties deviceProperties = {};
        vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);

        uint32_t deviceExtensionCount = 0;
        vkEnumerateDeviceExtensionProperties(
            physicalDevice,
            nullptr,
            &deviceExtensionCount,
            nullptr
        );
        vector<VkExtensionProperties> extensionProperties(deviceExtensionCount);
        vkEnumerateDeviceExtensionProperties(
            physicalDevice,
            nullptr,
            &deviceExtensionCount,
            extensionProperties.data()
        );
        bool hasSwapChain = false;
        string swapExtensionName(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
        for (auto extension: extensionProperties) {
            string name(extension.extensionName);
            if (name == swapExtensionName) {
                hasSwapChain = true;
            }
        }
        if (!hasSwapChain) {
            continue;
        }

        uint32_t queueFamilyPropertyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(
            physicalDevice, &queueFamilyPropertyCount, nullptr
        );
        vector<VkQueueFamilyProperties> queueFamilyPropertySets(
            queueFamilyPropertyCount
        );
        vkGetPhysicalDeviceQueueFamilyProperties(
            physicalDevice, &queueFamilyPropertyCount,
            queueFamilyPropertySets.data()
        );

        for (uint32_t index = 0; index < queueFamilyPropertyCount; index++) {
            auto queueFamilyProperties = queueFamilyPropertySets[index];
            if (queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                hasGraphicsQueue = true;
                _gfxFamily = index;
            }
            vkGetPhysicalDeviceSurfaceSupportKHR(
                physicalDevice,
                index,
                _surface,
                &hasPresentQueue
            );
            if (hasPresentQueue) {
                _presentFamily = index;
            }
        }

        if (hasGraphicsQueue && hasPresentQueue) {
            _physicalDevice = physicalDevice;
            LOG(INFO) << "selected physical device "
                      << deviceProperties.deviceName;
            return;
        }
    }

    throw runtime_error("no suitable physical device found");
}

void VulkanApplication::
initDeviceAndQueues() {
    float queuePriority = 1.f;

    vector<VkDeviceQueueCreateInfo> queueCreateInfos;

    VkDeviceQueueCreateInfo gfxQueueCreateInfo = {};
    gfxQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    gfxQueueCreateInfo.queueCount = 1;
    gfxQueueCreateInfo.queueFamilyIndex = _gfxFamily;
    gfxQueueCreateInfo.pQueuePriorities = &queuePriority;
    queueCreateInfos.push_back(gfxQueueCreateInfo);

    VkDeviceQueueCreateInfo presentQueueCreateInfo = {};
    presentQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    presentQueueCreateInfo.queueCount = 1;
    presentQueueCreateInfo.queueFamilyIndex = _presentFamily;
    presentQueueCreateInfo.pQueuePriorities = &queuePriority;
    queueCreateInfos.push_back(presentQueueCreateInfo);

    vector<char*> extensions({ VK_KHR_SWAPCHAIN_EXTENSION_NAME });

    VkDeviceCreateInfo deviceCreateInfo = {};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(
        queueCreateInfos.size()
    );
    deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
    deviceCreateInfo.enabledExtensionCount = (uint32_t)extensions.size();
    deviceCreateInfo.ppEnabledExtensionNames = extensions.data();

    auto result = vkCreateDevice(
        _physicalDevice,
        &deviceCreateInfo,
        nullptr,
        &_device
    );

    checkSuccess(
        result,
        "could not create device"
    );
    LOG(INFO) << "created device";

    vkGetDeviceQueue(_device, _gfxFamily, 0, &_gfxQueue);
    LOG(INFO) << "retrieved gfx queue";
    vkGetDeviceQueue(_device, _presentFamily, 0, &_presentQueue);
    LOG(INFO) << "retrieved present queue";
}

void VulkanApplication::
initSwapChain() {
    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    auto result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
        _physicalDevice,
        _surface,
        &surfaceCapabilities
    );
    checkSuccess(result, "could not query surface capabilities");
    if (!(surfaceCapabilities.supportedUsageFlags &
          VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)) {
        throw runtime_error("surface does not support color attachment");
    }
    if (!(surfaceCapabilities.supportedCompositeAlpha &
          VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR)) {
        throw runtime_error("surface does not support opaque compositing");
    }

    uint32_t surfaceFormatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(
        _physicalDevice,
        _surface,
        &surfaceFormatCount,
        nullptr
    );
    vector<VkSurfaceFormatKHR> surfaceFormats(surfaceFormatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(
        _physicalDevice,
        _surface,
        &surfaceFormatCount,
        surfaceFormats.data()
    );

    uint32_t presentModeCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(
        _physicalDevice,
        _surface,
        &presentModeCount,
        nullptr
    );
    vector<VkPresentModeKHR> presentModes(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(
        _physicalDevice,
        _surface,
        &presentModeCount,
        presentModes.data()
    );
    VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
    for (auto availablePresentMode: presentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
        }
    }

    VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = _surface;
    createInfo.minImageCount = surfaceCapabilities.minImageCount;
    createInfo.imageExtent = surfaceCapabilities.currentExtent;
    createInfo.oldSwapchain = VK_NULL_HANDLE;
    createInfo.imageFormat = surfaceFormats[0].format;
    createInfo.imageColorSpace = surfaceFormats[0].colorSpace;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    createInfo.presentMode = presentMode;
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.preTransform = surfaceCapabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.clipped = VK_FALSE;

    result = vkCreateSwapchainKHR(
        _device,
        &createInfo,
        nullptr,
        &_swapChain
    );
    checkSuccess(result, "could not create swap chain");

    uint32_t swapImageCount = 0;
    vkGetSwapchainImagesKHR(
        _device,
        _swapChain,
        &swapImageCount,
        nullptr
    );
    _swapImages.resize(swapImageCount);
    result = vkGetSwapchainImagesKHR(
        _device,
        _swapChain,
        &swapImageCount,
        _swapImages.data()
    );
    checkSuccess(
        result,
        "could not fetch swap images"
    );

    LOG(INFO) << "created swap chain";
}

void VulkanApplication::
present() {
    VkSemaphore semaphore;
    VkSemaphoreCreateInfo semaphoreCreateInfo = {};
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    vkCreateSemaphore(
        _device,
        &semaphoreCreateInfo,
        nullptr,
        &semaphore
    );

    uint32_t imageIndex = 0;
    VkResult result = vkAcquireNextImageKHR(
        _device,
        _swapChain,
        0,
        semaphore,
        VK_NULL_HANDLE,
        &imageIndex
    );
    checkSuccess(
        result,
        "could not acquire swap chain image"
    );

    VkCommandPoolCreateInfo commandPoolCreateInfo = {};
    commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandPoolCreateInfo.queueFamilyIndex = _presentFamily;
    result = vkCreateCommandPool(
        _device,
        &commandPoolCreateInfo,
        nullptr,
        &_presentCommandPool
    );
    checkSuccess(
        result,
        "could not create presentation queue command pool"
    );

    VkCommandBuffer commandBuffer;
    VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
    commandBufferAllocateInfo.sType = 
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocateInfo.commandPool = _presentCommandPool;
    commandBufferAllocateInfo.commandBufferCount = 1;
    commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    result = vkAllocateCommandBuffers(
        _device,
        &commandBufferAllocateInfo,
        &commandBuffer
    );
    checkSuccess(
        result,
        "could not create command buffer for presentation"
    );

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    result = vkBeginCommandBuffer(commandBuffer, &beginInfo);
    checkSuccess(
        result,
        "could not begin command"
    );

    VkImageMemoryBarrier imageBarrier = {};
    imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    imageBarrier.image = _swapImages[imageIndex];
    imageBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    imageBarrier.srcQueueFamilyIndex = _presentFamily;
    imageBarrier.dstQueueFamilyIndex = _presentFamily;
    imageBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageBarrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
    imageBarrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

    vkCmdPipelineBarrier(
        commandBuffer,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        0,
        0,
        nullptr,
        0,
        nullptr,
        1,
        &imageBarrier
    );

    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submit = {};
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &commandBuffer;
    submit.waitSemaphoreCount = 1;
    submit.pWaitSemaphores = &semaphore;
    vkQueueSubmit(
        _presentQueue,
        1,
        &submit,
        VK_NULL_HANDLE
    );

    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &_swapChain;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &semaphore;
    presentInfo.pImageIndices = &imageIndex;
    result = vkQueuePresentKHR(
        _presentQueue,
        &presentInfo
    );
    checkSuccess(
        result,
        "could not enqueue image for presentation"
    );

    vkFreeCommandBuffers(
        _device,
        _presentCommandPool,
        1,
        &commandBuffer
    );
    vkDestroySemaphore(_device, semaphore, nullptr);
    vkDestroyCommandPool(_device, _presentCommandPool, nullptr);
}
