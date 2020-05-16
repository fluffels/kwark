#include "VulkanApplication.h"
#include "VulkanCommandBuffer.h"
#include "VulkanUtils.h"

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
    } else if (flags == VK_DEBUG_REPORT_DEBUG_BIT_EXT) {
        LOG(DEBUG) << "[" << layerPrefix << "] " << msg;
    } else {
        LOG(INFO) << "[" << layerPrefix << "] " << msg;
    }
    return VK_FALSE;
}

VulkanApplication::
VulkanApplication(const Platform& platform,
                  Camera* camera,
                  vector<Vertex>& mesh,
                  Atlas* atlas):
        _enabledExtensions({
            VK_KHR_SURFACE_EXTENSION_NAME,
            VK_EXT_DEBUG_REPORT_EXTENSION_NAME
        }),
        _enabledLayers({ "VK_LAYER_KHRONOS_validation" }),
        _shouldResize(false),
        _atlas(atlas),
        _camera(camera),
        _mesh(mesh) {
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
    _surfaceCapabilities = getSurfaceCapabilities();
    checkSurfaceCapabilities();
    createDeviceAndQueues();
    getMemories();

    createSwapChain();
    createCommandPools();
    depth = createVulkanDepthBuffer(
        _device,
        _memories,
        _swapChainExtent,
        _gfxFamily
    );
    uploadTextures();

    getSwapImagesAndImageViews();
    createRenderPass();
    createFramebuffers();

    _vertexShader = createShaderModule("shaders/default.vert.spv");
    _fragmentShader = createShaderModule("shaders/default.frag.spv");
    createPipeline(_vertexShader, _fragmentShader);

    initCamera();
    createUniformBuffer();
    allocateUniformBuffer();
    uploadUniformData();

    createDescriptorPool();
    allocateDescriptorSet();
    updateDescriptorSet();

    createVertexBuffer();
    allocateVertexBuffer();
    uploadVertexData();

    createSwapCommandBuffers();
    recordCommandBuffers();

    createSemaphores();
}

VulkanApplication::
~VulkanApplication() {
    vkDeviceWaitIdle(_device);

    vkDestroySemaphore(_device, _imageReady, nullptr);
    vkDestroySemaphore(_device, _presentReady, nullptr);

    vkDestroyCommandPool(_device, _graphicsCommandPool, nullptr);
    vkDestroyCommandPool(_device, _transientCommandPool, nullptr);

    vkFreeMemory(_device, _vertexMemory, nullptr);
    vkDestroyBuffer(_device, _vertexBuffer, nullptr);

    vkFreeMemory(_device, _uniformMemory, nullptr);
    vkDestroyBuffer(_device, _uniformBuffer, nullptr);

    vkDestroyPipeline(_device, _pipeline, nullptr);
    vkDestroyDescriptorPool(_device, _descriptorPool, nullptr);
    vkDestroyPipelineLayout(_device, _layout, nullptr);
    vkDestroyDescriptorSetLayout(_device, _descriptorSetLayout, nullptr);
    vkDestroyShaderModule(_device, _fragmentShader, nullptr);
    vkDestroyShaderModule(_device, _vertexShader, nullptr);

    destroyFramebuffers();
    destroySwapImageViews();
    vkDestroyRenderPass(_device, _renderPass, nullptr);
    destroySwapchain(_swapChain);

    destroyVulkanImage(_device, depth);
    for (auto& sampler: samplers) {
        destroyVulkanSampler(_device, sampler);
    }

    vkDestroySurfaceKHR(_instance, _surface, nullptr);
    vkDestroyDevice(_device, nullptr);
    auto vkDestroyDebugReportCallbackEXT =
        (PFN_vkDestroyDebugReportCallbackEXT)
        vkGetInstanceProcAddr(_instance, "vkDestroyDebugReportCallbackEXT");
    vkDestroyDebugReportCallbackEXT(_instance, _debugCallback, nullptr);
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

VkSurfaceCapabilitiesKHR VulkanApplication::
getSurfaceCapabilities() {
    VkSurfaceCapabilitiesKHR surfaceCapabilities;

    auto result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
        _physicalDevice,
        _surface,
        &surfaceCapabilities
    );
    checkSuccess(result, "could not query surface capabilities");

    return surfaceCapabilities;
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
                &_debugCallback
            ),
            "couldn't create debug callback"
        );
        LOG(INFO) << "created debug callback";
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
                VkBool32 isPresentQueue;
                vkGetPhysicalDeviceSurfaceSupportKHR(
                    physicalDevice,
                    index,
                    _surface,
                    &isPresentQueue
                );
                if (isPresentQueue) {
                    hasGraphicsQueue = true;
                    _gfxFamily = index;
                }
            }
        }

        if (hasGraphicsQueue) {
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
}

void VulkanApplication::
createSwapChain() {
    VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = _surface;
    createInfo.minImageCount = _surfaceCapabilities.minImageCount;
    createInfo.imageExtent = _surfaceCapabilities.currentExtent;
    createInfo.oldSwapchain = VK_NULL_HANDLE;
    createInfo.imageFormat = _swapImageFormat;
    createInfo.imageColorSpace = _swapImageColorSpace;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    createInfo.presentMode = _presentMode;
    // createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.preTransform = _surfaceCapabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.clipped = VK_FALSE;
    // createInfo.queueFamilyIndexCount = 1;
    // createInfo.pQueueFamilyIndices = &_presentFamily;

    auto result = vkCreateSwapchainKHR(
        _device,
        &createInfo,
        nullptr,
        &_swapChain
    );
    checkSuccess(result, "could not create swap chain");
    LOG(INFO) << "created swap chain";
}

void VulkanApplication::
createRenderPass() {
    vector<VkAttachmentDescription> attachmentDescriptions;
    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = _swapImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    attachmentDescriptions.push_back(colorAttachment);
    
    VkAttachmentDescription depthAttachment = {};
    depthAttachment.format = VK_FORMAT_D32_SFLOAT;
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout =
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    attachmentDescriptions.push_back(depthAttachment);

    vector<VkAttachmentReference> colorAttachmentReferences;
    VkAttachmentReference colorAttachmentReference = {};
    colorAttachmentReference.attachment = 0;
    colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colorAttachmentReferences.push_back(colorAttachmentReference);

    VkAttachmentReference depthAttachmentReference = {};
    depthAttachmentReference.attachment = 1;
    depthAttachmentReference.layout =
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    vector<VkSubpassDescription> subpassDescriptions;
    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = (uint32_t)colorAttachmentReferences.size();
    subpass.pColorAttachments = colorAttachmentReferences.data();
    subpass.pDepthStencilAttachment = &depthAttachmentReference;
    subpassDescriptions.push_back(subpass);

    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
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
        VkImageView imageViews[] = { imageView, depth.view };
        VkFramebufferCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        createInfo.attachmentCount = 2;
        createInfo.pAttachments = imageViews;
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
    vector<VkDescriptorSetLayoutBinding> descriptorSetLayoutBindings(2);

    descriptorSetLayoutBindings[0].binding = 0;
    descriptorSetLayoutBindings[0].descriptorCount = 1;
    descriptorSetLayoutBindings[0].descriptorType =
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorSetLayoutBindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    
    descriptorSetLayoutBindings[1].binding = 1;
    descriptorSetLayoutBindings[1].descriptorCount =
        samplers.size();
    descriptorSetLayoutBindings[1].descriptorType =
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorSetLayoutBindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo uniformDescriptorSetLayoutCreateInfo = {};
    uniformDescriptorSetLayoutCreateInfo.sType =
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    uniformDescriptorSetLayoutCreateInfo.bindingCount =
        (uint32_t)descriptorSetLayoutBindings.size();
    uniformDescriptorSetLayoutCreateInfo.pBindings =
        descriptorSetLayoutBindings.data();
    
    vkCreateDescriptorSetLayout(
        _device,
        &uniformDescriptorSetLayoutCreateInfo,
        nullptr,
        &_descriptorSetLayout
    );

    vector<VkDescriptorSetLayout> descriptorSetLayouts;
    descriptorSetLayouts.push_back(_descriptorSetLayout);

    VkPushConstantRange pushConstants;
    pushConstants.offset = 0;
    pushConstants.size = 32 * 3;
    pushConstants.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
    pipelineLayoutCreateInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.setLayoutCount =
        (uint32_t)descriptorSetLayouts.size();
    pipelineLayoutCreateInfo.pSetLayouts = descriptorSetLayouts.data();
    pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
    pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstants;

    auto result = vkCreatePipelineLayout(
        _device,
        &pipelineLayoutCreateInfo,
        nullptr,
        &_layout
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
    vertexInputState.vertexBindingDescriptionCount = 1;
    vertexInputState.pVertexBindingDescriptions =
        &inputBindingDescription;
    vertexInputState.vertexAttributeDescriptionCount =
        (uint32_t)inputAttributeDescriptions.size();
    vertexInputState.pVertexAttributeDescriptions =
        inputAttributeDescriptions.data();
    
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo = {};
    inputAssemblyStateCreateInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyStateCreateInfo.topology =
        VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
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

    VkPipelineDepthStencilStateCreateInfo depthStencilCreateInfo = {};
    depthStencilCreateInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencilCreateInfo.depthTestEnable = VK_TRUE;
    depthStencilCreateInfo.depthWriteEnable = VK_TRUE;
    depthStencilCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS;
    // TODO(jan): Experiment with enabling this for better performance.
    depthStencilCreateInfo.depthBoundsTestEnable = VK_FALSE;
    depthStencilCreateInfo.stencilTestEnable = VK_FALSE;

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
    pipelineCreateInfo.pDepthStencilState = &depthStencilCreateInfo;
    pipelineCreateInfo.renderPass = _renderPass;
    pipelineCreateInfo.layout = _layout;
    pipelineCreateInfo.subpass = 0;
    
    result = vkCreateGraphicsPipelines(
        _device,
        VK_NULL_HANDLE,
        1,
        &pipelineCreateInfo,
        nullptr,
        &_pipeline
    );
    checkSuccess(
        result,
        "could not create pipeline"
    );
    LOG(INFO) << "created pipeline";
}

void VulkanApplication::
createDescriptorPool() {
    VkDescriptorPoolSize poolSize = {};
    poolSize.descriptorCount = 1;
    poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

    VkDescriptorPoolCreateInfo createInfo = {};
    createInfo.sType =
        VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    createInfo.maxSets = 1;
    createInfo.poolSizeCount = 1;
    createInfo.pPoolSizes = &poolSize;

    auto result = vkCreateDescriptorPool(
        _device,
        &createInfo,
        nullptr,
        &_descriptorPool
    );
    checkSuccess(result, "could not create descriptor pool");
}

void VulkanApplication::
allocateDescriptorSet() {
    VkDescriptorSetAllocateInfo allocateInfo = {};
    allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocateInfo.descriptorPool = _descriptorPool;
    allocateInfo.descriptorSetCount = 1;
    allocateInfo.pSetLayouts = &_descriptorSetLayout;

    auto result = vkAllocateDescriptorSets(
        _device,
        &allocateInfo,
        &_descriptorSet
    );
    checkSuccess(result, "could not allocate descriptor set");
}

void VulkanApplication::
updateDescriptorSet() {
    VkDescriptorBufferInfo bufferInfo;
    bufferInfo.buffer = _uniformBuffer;
    bufferInfo.offset = 0;
    bufferInfo.range = VK_WHOLE_SIZE;

    VkWriteDescriptorSet writeSets[2] = {};

    writeSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeSets[0].descriptorCount = 1;
    writeSets[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writeSets[0].dstBinding = 0;
    writeSets[0].dstSet = _descriptorSet;
    writeSets[0].pBufferInfo = &bufferInfo;

    auto samplerCount = samplers.size();
    vector<VkDescriptorImageInfo> imageInfos(samplerCount);
    for (int i = 0; i < samplerCount; i++) {
        auto& imageInfo = imageInfos[i];
        auto& sampler = samplers[i];
        imageInfo.imageView = sampler.image.view;
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.sampler = sampler.handle;
    }

    writeSets[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeSets[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writeSets[1].dstSet = _descriptorSet;
    writeSets[1].dstBinding = 1;
    writeSets[1].descriptorCount = (uint32_t)imageInfos.size();
    writeSets[1].pImageInfo = imageInfos.data();

    vkUpdateDescriptorSets(
        _device,
        2,
        writeSets,
        0,
        nullptr
    );
}

void VulkanApplication::
createCommandPools() {
    _graphicsCommandPool = createCommandPool(_device, _gfxFamily);
    _transientCommandPool = createCommandPool(_device, _gfxFamily, true);
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

VkBuffer VulkanApplication::
createBuffer(VkBufferUsageFlags usage, uint32_t size) {
    VkBuffer buffer;

    VkBufferCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.usage = usage;
    createInfo.queueFamilyIndexCount = 1;
    createInfo.pQueueFamilyIndices = &_gfxFamily;
    createInfo.size = size;

    auto result = vkCreateBuffer(
        _device,
        &createInfo,
        nullptr,
        &buffer
    );

    checkSuccess(
        result,
        "could not create vertex buffer"
    );
    LOG(INFO) << "created vertex buffer";

    return buffer;
}

void VulkanApplication::
createVertexBuffer() {
    _vertexBuffer = createBuffer(
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        (uint32_t)(_mesh.size() * sizeof(Vertex))
    );
}

void VulkanApplication::
createUniformBuffer() {
    _uniformBuffer = createBuffer(
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        1024
    );
}

VkDeviceMemory VulkanApplication::
allocateBuffer(VkBuffer buffer) {
    VkDeviceMemory memory;

    auto requirements = getMemoryRequirements(_device, buffer);
    auto extraFlags = (
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    );
    auto typeIndex = selectMemoryTypeIndex(_memories, requirements, extraFlags);

    VkMemoryAllocateInfo allocateInfo = {};
    allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocateInfo.allocationSize = requirements.size;
    allocateInfo.memoryTypeIndex = typeIndex;

    auto result = vkAllocateMemory(
        _device,
        &allocateInfo,
        nullptr,
        &memory
    );
    checkSuccess(
        result,
        "could not allocate buffer"
    );

    vkBindBufferMemory(
        _device,
        buffer,
        memory,
        0
    );

    return memory;
}

void VulkanApplication::
allocateUniformBuffer() {
    _uniformMemory = allocateBuffer(
        _uniformBuffer
    );
}

void VulkanApplication::
allocateVertexBuffer() {
    _vertexMemory = allocateBuffer(
        _vertexBuffer
    );
}

void VulkanApplication::
uploadUniformData() {
    auto mvp = _camera->get();
    auto dst = mapMemory(_device, _uniformBuffer, _uniformMemory);
    memcpy(dst, &mvp, sizeof(mvp));
    unMapMemory(_device, _uniformMemory);
}

void VulkanApplication::
uploadVertexData() {
    void* data = mapMemory(_device, _vertexBuffer, _vertexMemory);
        memcpy(data, _mesh.data(), sizeof(Vertex)*_mesh.size());
    unMapMemory(_device, _vertexMemory);
}

void VulkanApplication::
uploadTextures() {
    auto textureCount = _atlas->textures.size();
    samplers.resize(textureCount);
    for (int idx = 0; idx < _atlas->textureHeaders.size(); idx++) {
        auto& header = _atlas->textureHeaders[idx];
        if ((header.width > 0) && (header.height > 0)) {
            auto texNum = _atlas->textureIDMap[idx];
            auto& texture = _atlas->textures[texNum];
            auto& sampler = samplers[texNum];
            uploadTexture(header, texture, sampler);
        }
    }
}

void VulkanApplication::
uploadTexture(
    TextureHeader& header,
    std::vector<uint8_t>& texture,
    VulkanSampler& sampler
) {
    VkExtent2D extent = { header.width, header.height };

    sampler = createVulkanSampler(
        _device,
        _memories,
        extent,
        _gfxFamily
    );
    void* memory = mapMemory(
        _device,
        sampler.image.handle,
        sampler.image.memory
    );
    memcpy(memory, texture.data(), texture.size() * sizeof(uint8_t));
    unMapMemory(_device, sampler.image.memory);

    auto commands = allocateCommandBuffer(_device, _transientCommandPool);
    beginCommandBuffer(commands);

    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.image = sampler.image.handle;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(
        commands,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        0,
        0,
        nullptr,
        0,
        nullptr,
        1,
        &barrier
    );

    endCommandBuffer(commands);

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commands;
    vkQueueSubmit(_gfxQueue, 1, &submitInfo, 0);
}

void VulkanApplication::
checkSurfaceCapabilities() {
    if (!(_surfaceCapabilities.supportedUsageFlags &
          VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)) {
        throw runtime_error("surface does not support color attachment");
    }
    if (!(_surfaceCapabilities.supportedCompositeAlpha &
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
                (format.format == VK_FORMAT_B8G8R8A8_SRGB)) {
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
    _presentMode = VK_PRESENT_MODE_FIFO_KHR;
    for (auto availablePresentMode: presentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            _presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
        }
    }

    _swapChainExtent = _surfaceCapabilities.currentExtent;
}

void VulkanApplication::
destroyFramebuffers() {
    for (auto framebuffer: _framebuffers) {
        vkDestroyFramebuffer(_device, framebuffer, nullptr);
    }
    _framebuffers.clear();
}

void VulkanApplication::
destroySwapImageViews() {
    for (auto imageView: _swapImageViews) {
        vkDestroyImageView(_device, imageView, nullptr);
    }
}

void VulkanApplication::
destroySwapchain(VkSwapchainKHR& target) {
    vkDestroySwapchainKHR(_device, target, nullptr);
}

void VulkanApplication::
getMemories() {
    vkGetPhysicalDeviceMemoryProperties(
        _physicalDevice,
        &_memories
    );
}

void VulkanApplication::
getSwapImagesAndImageViews() {
    uint32_t swapImageCount = 0;
    vkGetSwapchainImagesKHR(
        _device,
        _swapChain,
        &swapImageCount,
        nullptr
    );
    _swapImages.resize(swapImageCount);
    auto result = vkGetSwapchainImagesKHR(
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
initCamera() {
    _camera->up = { 0, 1, 0 };
    _camera->setAR(_swapChainExtent.width, _swapChainExtent.height);
    _camera->setFOV(45);
    _camera->nearz = 1.f;
    _camera->farz = 5000.f;
}

void VulkanApplication::
present() {
    uint32_t imageIndex = 0;
    auto result = vkAcquireNextImageKHR(
        _device,
        _swapChain,
        std::numeric_limits<uint64_t>::max(),
        _imageReady,
        VK_NULL_HANDLE,
        &imageIndex
    );
    if ((result == VK_SUBOPTIMAL_KHR) ||
            (result == VK_ERROR_OUT_OF_DATE_KHR) ||
            _shouldResize) {
        _shouldResize = false;
        resizeSwapChain();
        return;
    } else if (result != VK_SUCCESS) {
        throw std::runtime_error("could not acquire next image");
    }

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
        _gfxQueue,
        &presentInfo
    );
    checkSuccess(
        result,
        "could not enqueue image for presentation"
    );

    uploadUniformData();
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

        VkClearValue colorClear;
        colorClear.color = {1.f, 1.f, 1.f, 1.f};
        VkClearValue depthClear;
        depthClear.depthStencil = { 1.f, 0 };
        VkClearValue clears[] = { colorClear, depthClear };

        VkRenderPassBeginInfo renderPassBeginInfo = {};
        renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassBeginInfo.clearValueCount = 2;
        renderPassBeginInfo.pClearValues = clears;
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

        vkCmdBindDescriptorSets(
            commandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            _layout,
            0,
            1,
            &_descriptorSet,
            0,
            nullptr
        );

        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(
            commandBuffer,
            0, 1,
            &_vertexBuffer,
            offsets
        );
        float color[3] = {0.f, 1.f, 1.f};
        vkCmdPushConstants(
            commandBuffer,
            _layout,
            VK_SHADER_STAGE_VERTEX_BIT,
            0,
            32 * 3,
            color
        );
        vkCmdDraw(
            commandBuffer,
            (uint32_t)_mesh.size() / 2, 1,
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
resize() {
    _shouldResize = true;
}

void VulkanApplication::
resizeSwapChain() {
    _surfaceCapabilities = getSurfaceCapabilities();
    checkSurfaceCapabilities();

    vkQueueWaitIdle(_gfxQueue);

    VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.oldSwapchain = _swapChain;
    createInfo.imageFormat = _swapImageFormat;
    createInfo.imageColorSpace = _swapImageColorSpace;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    createInfo.presentMode = _presentMode;
    createInfo.minImageCount = _surfaceCapabilities.minImageCount;
    createInfo.imageExtent = _swapChainExtent;
    createInfo.preTransform = _surfaceCapabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.clipped = VK_FALSE;
    createInfo.surface = _surface;
    
    auto result = vkCreateSwapchainKHR(
        _device,
        &createInfo,
        nullptr,
        &_swapChain
    );
    checkSuccess(result, "could not resize swap chain");
    LOG(INFO) << "swap chain resized";

    vkDestroyPipeline(_device, _pipeline, nullptr);
    vkDestroyPipelineLayout(_device, _layout, nullptr);
    vkDestroyDescriptorSetLayout(_device, _descriptorSetLayout, nullptr);
    vkDestroyRenderPass(_device, _renderPass, nullptr);
    vkDestroySemaphore(_device, _imageReady, nullptr);
    vkDestroySemaphore(_device, _presentReady, nullptr);
    destroyFramebuffers();
    destroySwapImageViews();
    _swapImages.clear();
    _swapImageViews.clear();
    _swapCommandBuffers.clear();
    destroySwapchain(createInfo.oldSwapchain);
    destroyVulkanImage(_device, depth);

    depth = createVulkanDepthBuffer(
        _device,
        _memories,
        _swapChainExtent,
        _gfxFamily
    );
    getSwapImagesAndImageViews();
    createRenderPass();
    createFramebuffers();
    createPipeline(_vertexShader, _fragmentShader);
    createSwapCommandBuffers();
    recordCommandBuffers();
    createSemaphores();
}
