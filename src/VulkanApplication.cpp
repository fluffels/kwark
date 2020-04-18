#include "VulkanApplication.h"

static VKAPI_ATTR VkBool32 VKAPI_CALL
debugCallback(
    VkDebugReportFlagsEXT flags,
    VkDebugReportObjectTypeEXT objType,
    uint64_t obj,
    size_t location,
    int32_t code,
    const char *layerPrefix,
    const char *msg,
    void *userData
) {
    if (flags == VK_DEBUG_REPORT_ERROR_BIT_EXT) {
        LOG(ERROR) << "[" << layerPrefix << "] " << msg;
    } else if (flags == VK_DEBUG_REPORT_WARNING_BIT_EXT) {
        LOG(WARNING) << "[" << layerPrefix << "] " << msg;
    } else {
        LOG(DEBUG) << "[" << layerPrefix << "] " << msg;
    }
    return VK_FALSE;
}

VulkanApplication::
VulkanApplication(const Platform& platform):
        _enabledExtensions({
            VK_KHR_SURFACE_EXTENSION_NAME,
            VK_EXT_DEBUG_REPORT_EXTENSION_NAME
        }),
        _enabledLayers({ "VK_LAYER_LUNARG_standard_validation" }) {
    auto platformRequiredExtensions = platform.getExtensions();
    _enabledExtensions.insert(
        _enabledExtensions.end(),
        platformRequiredExtensions.begin(),
        platformRequiredExtensions.end()
    );

    vkEnumerateInstanceVersion(&_version);
    checkVersion(_version);

    createVulkanInstance();
    createDebugCallback();
    _surface = platform.getSurface(_instance);
    createPhysicalDevice();
    createDeviceAndQueues();
    createSwapChain();
    createRenderPass();
    createFramebuffers();

    auto vertexShader = createShaderModule("shaders/default.vert.spv");
    auto fragmentShader = createShaderModule("shaders/default.frag.spv");
    createPipeline(vertexShader, fragmentShader);
    vkDestroyShaderModule(_device, fragmentShader, nullptr);
    vkDestroyShaderModule(_device, vertexShader, nullptr);

    loadVertexBuffer();

    createGraphicsCommandPool();
    createSwapCommandBuffers();
    recordCommandBuffers();

    createSemaphores();
}

VulkanApplication::
~VulkanApplication() {
    vkDestroySemaphore(_device, _imageReady, nullptr);
    vkDestroySemaphore(_device, _presentReady, nullptr);
    for (auto commandBuffer: _swapCommandBuffers) {
        vkFreeCommandBuffers(
            _device,
            _graphicsCommandPool,
            (uint32_t)_swapCommandBuffers.size(),
            _swapCommandBuffers.data()
        );
    }
    vkDestroyBuffer(_device, _vertexBuffer, nullptr);
    vkDestroyPipeline(_device, _pipeline, nullptr);
    for (auto framebuffer: _framebuffers) {
        vkDestroyFramebuffer(_device, framebuffer, nullptr);
    }
    for (auto imageView: _swapImageViews) {
        vkDestroyImageView(_device, imageView, nullptr);
    }
    vkDestroyRenderPass(_device, _renderPass, nullptr);
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
createVulkanInstance() {
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
createDebugCallback() {
    VkDebugReportCallbackEXT callbackDebug;
    VkDebugReportCallbackCreateInfoEXT debugReportCallbackCreateInfo = {};
    debugReportCallbackCreateInfo.sType =
        VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
    debugReportCallbackCreateInfo.flags =
        VK_DEBUG_REPORT_ERROR_BIT_EXT |
        VK_DEBUG_REPORT_WARNING_BIT_EXT |
        VK_DEBUG_REPORT_DEBUG_BIT_EXT;
    debugReportCallbackCreateInfo.pfnCallback = debugCallback;
    auto create =
        (PFN_vkCreateDebugReportCallbackEXT)
        vkGetInstanceProcAddr(_instance, "vkCreateDebugReportCallbackEXT");
    if (create == nullptr) {
        LOG(WARNING) << "couldn't load debug callback creation function";
    } else {
        checkSuccess(
            create(
                _instance,
                &debugReportCallbackCreateInfo,
                nullptr,
                &callbackDebug
            ),
            "couldn't create debug callback"
        );
    }
}

void VulkanApplication::
createPhysicalDevice() {
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
createDeviceAndQueues() {
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
createSwapChain() {
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

    _swapImageFormat = surfaceFormats[0].format;
    _swapImageColorSpace = surfaceFormats[0].colorSpace;
    for (auto format: surfaceFormats) {
        if ((format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) &&
                (format.format == VK_FORMAT_B8G8R8_SRGB)) {
            _swapImageFormat = format.format;
            _swapImageColorSpace = format.colorSpace;
            break;
        }
    }

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

    _swapChainExtent = surfaceCapabilities.currentExtent;

    VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = _surface;
    createInfo.minImageCount = surfaceCapabilities.minImageCount;
    createInfo.imageExtent = surfaceCapabilities.currentExtent;
    createInfo.oldSwapchain = VK_NULL_HANDLE;
    createInfo.imageFormat = _swapImageFormat;
    createInfo.imageColorSpace = _swapImageColorSpace;
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
    LOG(INFO) << "created swap chain";

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
    LOG(INFO) << "fetched swap images";

    for (auto image: _swapImages) {
        VkImageViewCreateInfo imageViewCreateInfo = {};
        imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewCreateInfo.image = image;
        imageViewCreateInfo.format = _swapImageFormat;
        imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageViewCreateInfo.subresourceRange.aspectMask =
            VK_IMAGE_ASPECT_COLOR_BIT;
        imageViewCreateInfo.subresourceRange.layerCount =
            VK_REMAINING_ARRAY_LAYERS;
        imageViewCreateInfo.subresourceRange.levelCount =
            VK_REMAINING_MIP_LEVELS;
        imageViewCreateInfo.components.a =
            VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.b =
            VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.g =
            VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.r =
            VK_COMPONENT_SWIZZLE_IDENTITY;

        VkImageView imageView = {};
        result = vkCreateImageView(
            _device,
            &imageViewCreateInfo,
            nullptr,
            &imageView
        );

        checkSuccess(
            result,
            "could not create swap image view"
        );

        _swapImageViews.push_back(imageView);

        LOG(INFO) << "created swap image view";
    }
}

void VulkanApplication::
createRenderPass() {
    vector<VkAttachmentDescription> attachmentDescriptions;
    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = _swapImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    attachmentDescriptions.push_back(colorAttachment);

    vector<VkAttachmentReference> colorAttachmentReferences;
    VkAttachmentReference colorAttachmentReference = {};
    colorAttachmentReference.attachment = 0;
    colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    vector<VkSubpassDescription> subpassDescriptions;
    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = (uint32_t)colorAttachmentReferences.size();
    subpass.pColorAttachments = colorAttachmentReferences.data();
    subpassDescriptions.push_back(subpass);

    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    createInfo.attachmentCount = (uint32_t)attachmentDescriptions.size();
    createInfo.pAttachments = attachmentDescriptions.data();
    createInfo.subpassCount = (uint32_t)subpassDescriptions.size();
    createInfo.pSubpasses = subpassDescriptions.data();
    createInfo.dependencyCount = 1;
    createInfo.pDependencies = &dependency;

    VkResult result = vkCreateRenderPass(
        _device,
        &createInfo,
        nullptr,
        &_renderPass
    );

    checkSuccess(
        result,
        "could not create subpass"
    );

    LOG(INFO) << "created render pass";
}

void VulkanApplication::
createFramebuffers() {
    for (auto imageView: _swapImageViews) {
        VkFramebufferCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        createInfo.attachmentCount = 1;
        createInfo.pAttachments = &imageView;
        createInfo.renderPass = _renderPass;
        createInfo.height = _swapChainExtent.height;
        createInfo.width = _swapChainExtent.width;
        createInfo.layers = 1;

        VkFramebuffer framebuffer;
        
        VkResult result = vkCreateFramebuffer(
            _device,
            &createInfo,
            nullptr,
            &framebuffer
        );

        checkSuccess(
            result,
            "could not create framebuffer"
        );

        LOG(INFO) << "created framebuffer";

        _framebuffers.push_back(framebuffer);
    }
}

VkShaderModule VulkanApplication::
createShaderModule(const string& path) {
    auto code = readFile(path);
    auto result = createShaderModule(code);
    return result;
}

VkShaderModule VulkanApplication::
createShaderModule(const vector<char>& code) {
    VkShaderModule shaderModule;

    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
    auto result = vkCreateShaderModule(
        _device,
        &createInfo,
        nullptr,
        &shaderModule
    );

    checkSuccess(
        result,
        "could not create shader module"
    );

    return shaderModule;
}

void VulkanApplication::createPipeline(
    VkShaderModule& vertexShaderModule,
    VkShaderModule& fragmentShaderModule
) {
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
    pipelineLayoutCreateInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.setLayoutCount = 0;
    pipelineLayoutCreateInfo.pushConstantRangeCount = 0;

    VkPipelineLayout layout = {};
    auto result = vkCreatePipelineLayout(
        _device,
        &pipelineLayoutCreateInfo,
        nullptr,
        &layout
    );
    checkSuccess(
        result,
        "could not create pipeline layout"
    );

    vector<VkPipelineShaderStageCreateInfo> shaderStageCreateInfos;
    if (vertexShaderModule != nullptr) {
        VkPipelineShaderStageCreateInfo vertexShaderStageCreateInfo = {};
        vertexShaderStageCreateInfo.sType =
            VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertexShaderStageCreateInfo.stage =
            VK_SHADER_STAGE_VERTEX_BIT;
        vertexShaderStageCreateInfo.module = vertexShaderModule;
        vertexShaderStageCreateInfo.pName = "main";
        shaderStageCreateInfos.push_back(vertexShaderStageCreateInfo);
    }
    if (fragmentShaderModule != nullptr) {
        VkPipelineShaderStageCreateInfo fragmentShaderStageCreateInfo = {};
        fragmentShaderStageCreateInfo.sType =
            VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragmentShaderStageCreateInfo.stage =
            VK_SHADER_STAGE_FRAGMENT_BIT;
        fragmentShaderStageCreateInfo.module = fragmentShaderModule;
        fragmentShaderStageCreateInfo.pName = "main";
        shaderStageCreateInfos.push_back(fragmentShaderStageCreateInfo);
    }

    auto inputBindingDescription = Vertex::getInputBindingDescription();
    auto inputAttributeDescriptions = Vertex::getInputAttributeDescriptions();

    VkPipelineVertexInputStateCreateInfo vertexInputState = {};
    vertexInputState.sType =
        VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputState.vertexBindingDescriptionCount = 0;
    vertexInputState.pVertexBindingDescriptions = nullptr;
    vertexInputState.vertexAttributeDescriptionCount = 0;
    vertexInputState.pVertexAttributeDescriptions = nullptr;
    
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo = {};
    inputAssemblyStateCreateInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyStateCreateInfo.topology =
        VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
    inputAssemblyStateCreateInfo.primitiveRestartEnable = VK_FALSE;
    
    // VkPipelineTessellationStateCreateInfo tessellationStateCreateInfo = {};
    // tessellationStateCreateInfo.sType =
        // VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;

    VkViewport viewport = {};
    viewport.height = (float)_swapChainExtent.height;
    viewport.width = (float)_swapChainExtent.width;
    viewport.minDepth = 0.f;
    viewport.maxDepth = 1.f; 
    viewport.x = 0.f;
    viewport.y = 0.f;

    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent = _swapChainExtent;

    VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {};
    viewportStateCreateInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportStateCreateInfo.viewportCount = 1;
    viewportStateCreateInfo.pViewports = &viewport;
    viewportStateCreateInfo.scissorCount = 1;
    viewportStateCreateInfo.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo = {};
    rasterizationStateCreateInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizationStateCreateInfo.frontFace =
        VK_FRONT_FACE_CLOCKWISE;
    rasterizationStateCreateInfo.cullMode =
        VK_CULL_MODE_BACK_BIT;
    rasterizationStateCreateInfo.lineWidth = 1.f;
    rasterizationStateCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizationStateCreateInfo.rasterizerDiscardEnable = VK_FALSE;
    rasterizationStateCreateInfo.depthClampEnable = VK_FALSE;
    rasterizationStateCreateInfo.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo = {};
    multisampleStateCreateInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampleStateCreateInfo.rasterizationSamples =
        VK_SAMPLE_COUNT_1_BIT;
    multisampleStateCreateInfo.sampleShadingEnable = VK_FALSE;
    multisampleStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampleStateCreateInfo.minSampleShading = 1.0f; // Optional
    multisampleStateCreateInfo.pSampleMask = nullptr; // Optional
    multisampleStateCreateInfo.alphaToCoverageEnable = VK_FALSE; // Optional
    multisampleStateCreateInfo.alphaToOneEnable = VK_FALSE; // Optional

    // VkPipelineDepthStencilStateCreateInfo*
    // VkPipelineDynamicStateCreateInfo*

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                                            VK_COLOR_COMPONENT_G_BIT |
                                            VK_COLOR_COMPONENT_B_BIT |
                                            VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlending = {};
    colorBlending.sType =
            VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

    VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
    pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineCreateInfo.stageCount = (uint32_t)shaderStageCreateInfos.size();
    pipelineCreateInfo.pStages = shaderStageCreateInfos.data();
    pipelineCreateInfo.pVertexInputState = &vertexInputState;
    pipelineCreateInfo.pInputAssemblyState = &inputAssemblyStateCreateInfo;
    pipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
    pipelineCreateInfo.pRasterizationState = &rasterizationStateCreateInfo;
    pipelineCreateInfo.pMultisampleState = &multisampleStateCreateInfo;
    pipelineCreateInfo.pColorBlendState = &colorBlending;
    pipelineCreateInfo.renderPass = _renderPass;
    pipelineCreateInfo.layout = layout;
    pipelineCreateInfo.subpass = 0;
    result = vkCreateGraphicsPipelines(
        _device,
        VK_NULL_HANDLE,
        1,
        &pipelineCreateInfo,
        nullptr,
        &_pipeline
    );

    vkDestroyPipelineLayout(_device, layout, nullptr);

    checkSuccess(
        result,
        "could not create pipeline"
    );

    LOG(INFO) << "created pipeline";
}

void VulkanApplication::
createGraphicsCommandPool() {
    VkCommandPoolCreateInfo commandPoolCreateInfo = {};
    commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandPoolCreateInfo.queueFamilyIndex = _gfxFamily;
    auto result = vkCreateCommandPool(
        _device,
        &commandPoolCreateInfo,
        nullptr,
        &_graphicsCommandPool
    );
    checkSuccess(
        result,
        "could not create graphics queue command pool"
    );
    LOG(INFO) << "created graphics queue command pool";
}

void VulkanApplication::
createPresentCommandPool() {
    VkCommandPoolCreateInfo commandPoolCreateInfo = {};
    commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandPoolCreateInfo.queueFamilyIndex = _presentFamily;
    auto result = vkCreateCommandPool(
        _device,
        &commandPoolCreateInfo,
        nullptr,
        &_presentCommandPool
    );
    checkSuccess(
        result,
        "could not create presentation queue command pool"
    );
    LOG(INFO) << "created presentation queue command pool";
}

void VulkanApplication::
createSwapCommandBuffers() {
    _swapCommandBuffers.resize(_swapImages.size());

    VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
    commandBufferAllocateInfo.sType = 
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocateInfo.commandPool = _graphicsCommandPool;
    commandBufferAllocateInfo.commandBufferCount = (uint32_t)_swapImages.size();
    commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    auto result = vkAllocateCommandBuffers(
        _device,
        &commandBufferAllocateInfo,
        _swapCommandBuffers.data()
    );

    checkSuccess(
        result,
        "could not create command buffers for swap images"
    );

    LOG(INFO) << "created command buffers for swap images";
}

void VulkanApplication::
loadVertexBuffer() {
    float vertices[] = {
        -1.f, 0.f, 0.f,
        0.f, 1.f, 0.f,
        1.f, 0.f, 0.f
    };

    VkBufferCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    createInfo.queueFamilyIndexCount = 1;
    createInfo.pQueueFamilyIndices = &_gfxFamily;
    createInfo.size = sizeof(vertices);

    auto result = vkCreateBuffer(
        _device,
        &createInfo,
        nullptr,
        &_vertexBuffer
    );

    checkSuccess(
        result,
        "could not create vertex buffer"
    );

    LOG(INFO) << "created vertex buffer";
}

void VulkanApplication::
recordCommandBuffers() {
    for (size_t swapIndex = 0; swapIndex < _swapImages.size(); swapIndex++) {
        auto commandBuffer = _swapCommandBuffers[swapIndex];

        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
        auto result = vkBeginCommandBuffer(
            commandBuffer,
            &beginInfo
        );
        checkSuccess(
            result,
            "could not begin command"
        );

        VkClearValue clearValue = {1.0f, 0.0f, 0.0f, 1.0f};

        VkRenderPassBeginInfo renderPassBeginInfo = {};
        renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassBeginInfo.clearValueCount = 1;
        renderPassBeginInfo.pClearValues = &clearValue;
        renderPassBeginInfo.framebuffer = _framebuffers[swapIndex];
        renderPassBeginInfo.renderArea.extent = _swapChainExtent;
        renderPassBeginInfo.renderArea.offset = {0, 0};
        renderPassBeginInfo.renderPass = _renderPass;

        vkCmdBeginRenderPass(
            commandBuffer,
            &renderPassBeginInfo,
            VK_SUBPASS_CONTENTS_INLINE
        );

        vkCmdBindPipeline(
            commandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            _pipeline
        );

        /*VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(
            commandBuffer,
            0, 1,
            &_vertexBuffer,
            offsets
        );*/
        vkCmdDraw(
            commandBuffer,
            3, 1,
            0, 0
        );

        vkCmdEndRenderPass(commandBuffer);

        result = vkEndCommandBuffer(commandBuffer);
        checkSuccess(
            result,
            "could not record command buffer"
        );
    }
}

void VulkanApplication::
createSemaphores() {
    VkSemaphoreCreateInfo semaphoreCreateInfo = {};
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    vkCreateSemaphore(
        _device,
        &semaphoreCreateInfo,
        nullptr,
        &_imageReady
    );

    vkCreateSemaphore(
        _device,
        &semaphoreCreateInfo,
        nullptr,
        &_presentReady
    );
}

void VulkanApplication::
present() {
    uint32_t imageIndex = 0;
    VkResult result = vkAcquireNextImageKHR(
        _device,
        _swapChain,
        std::numeric_limits<uint64_t>::max(),
        _imageReady,
        VK_NULL_HANDLE,
        &imageIndex
    );
    checkSuccess(
        result,
        "could not acquire swap chain image"
    );

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &_swapCommandBuffers[imageIndex];
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &_imageReady;
    VkPipelineStageFlags waitStages[] = {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
    };
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &_presentReady;
    vkQueueSubmit(_gfxQueue, 1, &submitInfo, nullptr);

    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &_swapChain;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &_presentReady;
    presentInfo.pImageIndices = &imageIndex;
    result = vkQueuePresentKHR(
        _presentQueue,
        &presentInfo
    );
    checkSuccess(
        result,
        "could not enqueue image for presentation"
    );
    LOG(INFO) << "enqueued image for presentation";
}
