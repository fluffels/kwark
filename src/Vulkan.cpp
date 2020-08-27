#pragma warning(disable: 4018)
#pragma warning(disable: 4267)

#include <stdexcept>

#include "easylogging++.h"

#include "Util.h"
#include "Vulkan.h"

using std::runtime_error;
using std::string;

void checkVersion(uint32_t version) {
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

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
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

void createDebugCallback(Vulkan& vk) {
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
        vkGetInstanceProcAddr(vk.handle, "vkCreateDebugReportCallbackEXT");
    if (create == nullptr) {
        LOG(WARNING) << "couldn't load debug callback creation function";
    } else {
        checkSuccess(
            create(
                vk.handle,
                &debugReportCallbackCreateInfo,
                nullptr,
                &vk.debugCallback
            )
        );
    }
}

void createVKInstance(Vulkan& vk) {
    uint32_t version;

    vkEnumerateInstanceVersion(&version);
    checkVersion(version);

    vk.extensions.insert(vk.extensions.begin(), VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
    vk.extensions.insert(vk.extensions.begin(), VK_KHR_SURFACE_EXTENSION_NAME);

    vk.layers.push_back("VK_LAYER_KHRONOS_validation");

    VkApplicationInfo app = {};
    app.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;

    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &app;

    createInfo.enabledLayerCount = vk.layers.size();
    auto enabledLayerNames = stringVectorToC(vk.layers);
    createInfo.ppEnabledLayerNames = enabledLayerNames;

    createInfo.enabledExtensionCount = vk.extensions.size();
    auto enabledExtensionNames = stringVectorToC(vk.extensions);
    createInfo.ppEnabledExtensionNames = enabledExtensionNames;
    
    checkSuccess(vkCreateInstance(&createInfo, nullptr, &vk.handle));

    createDebugCallback(vk);

    delete[] enabledLayerNames;
    delete[] enabledExtensionNames;
}

void pickGPU(Vulkan& vk) {
    uint32_t gpuCount = 0;
    checkSuccess(vkEnumeratePhysicalDevices(vk.handle, &gpuCount, nullptr));
    LOG(INFO) << gpuCount << " physical device(s)";

    vector<VkPhysicalDevice> gpus(gpuCount);
    checkSuccess(vkEnumeratePhysicalDevices(vk.handle, &gpuCount, gpus.data()));

    for (auto gpu: gpus) {
        bool hasGraphicsQueue = false;

        VkPhysicalDeviceProperties props = {};
        vkGetPhysicalDeviceProperties(gpu, &props);

        uint32_t extensionCount = 0;
        vkEnumerateDeviceExtensionProperties(
            gpu,
            nullptr,
            &extensionCount,
            nullptr
        );
        vector<VkExtensionProperties> extensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(
            gpu,
            nullptr,
            &extensionCount,
            extensions.data()
        );
        bool hasSwapChain = false;
        string swapExtensionName(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
        for (auto extension: extensions) {
            string name(extension.extensionName);
            if (name == swapExtensionName) {
                hasSwapChain = true;
            }
        }
        if (!hasSwapChain) {
            continue;
        }

        uint32_t familyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(gpu, &familyCount, nullptr);
        vector<VkQueueFamilyProperties> families(familyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(
            gpu,
            &familyCount,
            families.data()
        );

        for (uint32_t index = 0; index < familyCount; index++) {
            auto& queue = families[index];
            if (queue.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                VkBool32 isPresentQueue;
                vkGetPhysicalDeviceSurfaceSupportKHR(
                    gpu,
                    index,
                    vk.swap.surface,
                    &isPresentQueue
                );
                if (isPresentQueue) {
                    hasGraphicsQueue = true;
                    vk.queueFamily = index;
                }
            }
        }

        if (hasGraphicsQueue) {
            vk.gpu = gpu;
            LOG(INFO) << "selected physical device " << props.deviceName;
            return;
        }
    }

    throw runtime_error("no suitable physical device found");
}

void createDevice(Vulkan& vk) {
    vector<VkDeviceQueueCreateInfo> queueCreateInfos;

    float prio = 1.f;
    VkDeviceQueueCreateInfo queue = {};
    queue.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue.queueCount = 1;
    queue.queueFamilyIndex = vk.queueFamily;
    queue.pQueuePriorities = &prio;
    queueCreateInfos.push_back(queue);

    vector<char*> extensions({ VK_KHR_SWAPCHAIN_EXTENSION_NAME });

    VkDeviceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(
        queueCreateInfos.size()
    );
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.enabledExtensionCount = (uint32_t)extensions.size();
    createInfo.ppEnabledExtensionNames = extensions.data();

    checkSuccess(vkCreateDevice(vk.gpu, &createInfo, nullptr, &vk.device));
    vkGetDeviceQueue(vk.device, vk.queueFamily, 0, &vk.queue);
}

void createRenderPass(Vulkan& vk, bool clear, VkRenderPass& renderPass) {
    vector<VkAttachmentDescription> attachments;
    VkAttachmentDescription color = {};
    color.format = vk.swap.format;
    color.samples = VK_SAMPLE_COUNT_1_BIT;
    color.loadOp = clear? VK_ATTACHMENT_LOAD_OP_CLEAR: VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    attachments.push_back(color);
    
    VkAttachmentDescription depth = {};
    depth.format = VK_FORMAT_D32_SFLOAT;
    depth.samples = VK_SAMPLE_COUNT_1_BIT;
    depth.loadOp = clear? VK_ATTACHMENT_LOAD_OP_CLEAR: VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depth.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depth.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depth.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    attachments.push_back(depth);

    vector<VkAttachmentReference> colorReferences;
    VkAttachmentReference colorReference = {};
    colorReference.attachment = 0;
    colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colorReferences.push_back(colorReference);

    VkAttachmentReference depthReference = {};
    depthReference.attachment = 1;
    depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    vector<VkSubpassDescription> subpasses;
    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = (uint32_t)colorReferences.size();
    subpass.pColorAttachments = colorReferences.data();
    subpass.pDepthStencilAttachment = &depthReference;
    subpasses.push_back(subpass);

    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    createInfo.attachmentCount = (uint32_t)attachments.size();
    createInfo.pAttachments = attachments.data();
    createInfo.subpassCount = (uint32_t)subpasses.size();
    createInfo.pSubpasses = subpasses.data();
    createInfo.dependencyCount = 1;
    createInfo.pDependencies = &dependency;

    checkSuccess(vkCreateRenderPass(
        vk.device, &createInfo, nullptr, &renderPass
    ));
}

void initVK(Vulkan& vk) {
    pickGPU(vk);
    createDevice(vk);
    initVKSwapChain(vk);
    vk.memories = getMemories(vk.gpu);
    createUniformBuffer(vk.device, vk.memories, vk.queueFamily, 1024, vk.mvp);
    createRenderPass(vk, true, vk.renderPass);
    createRenderPass(vk, false, vk.renderPassNoClear);
    vk.depth = createVulkanDepthBuffer(
        vk.device,
        vk.memories,
        vk.swap.extent,
        vk.queueFamily
    );
    createFramebuffers(vk);
    vk.cmdPool = createCommandPool(vk.device, vk.queueFamily);
    vk.cmdPoolTransient = createCommandPool(vk.device, vk.queueFamily, true);
}

#include "VulkanBuffer.cpp"
#include "VulkanCommandBuffer.cpp"
#include "VulkanDescriptors.cpp"
#include "VulkanImage.cpp"
#include "VulkanMemory.cpp"
#include "VulkanMesh.cpp"
#include "VulkanPipeline.cpp"
#include "VulkanSwapChain.cpp"
#include "VulkanSynch.cpp"
