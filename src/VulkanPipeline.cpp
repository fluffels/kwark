#pragma warning(disable: 4267)

#include "SPIRV-Reflect/spirv_reflect.h"

#include <io.h>
#include <map>

#include "util.h"
#include "FileSystem.h"
#include "Vertex.h"
#include "VulkanPipeline.h"

using std::map;

void createDescriptorLayout(
    Vulkan& vk,
    vector<VulkanShader>& shaders,
    VulkanPipeline& pipeline
) {
    vector<VkDescriptorSetLayoutBinding> bindings;

    map<uint32_t, VkDescriptorSetLayoutBinding*> bindingDescMap;

    for (auto& shader: shaders) {
        for (auto& set: shader.sets) {
            for (uint32_t i = set->set; i < set->binding_count; i++) {
                auto& spirv = *(set->bindings[i]);

                auto existingDesc = bindingDescMap[spirv.binding];
                if (existingDesc) {
                    existingDesc->stageFlags |= shader.reflect.shader_stage;
                } else {
                    auto& desc = bindings.emplace_back();
                    bindingDescMap[spirv.binding] = &desc;
                    desc.binding = spirv.binding;
                    desc.descriptorCount = spirv.count;
                    desc.descriptorType = (VkDescriptorType)spirv.descriptor_type;
                    desc.stageFlags = shader.reflect.shader_stage;
                }
            }
        }
    }

    VkDescriptorSetLayoutCreateInfo descriptors = {};
    descriptors.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptors.bindingCount = (uint32_t)bindings.size();
    descriptors.pBindings = bindings.data();
    
    checkSuccess(vkCreateDescriptorSetLayout(
        vk.device,
        &descriptors,
        nullptr,
        &pipeline.descriptorLayout
    ));
}

void createDescriptorPool(
    Vulkan& vk,
    vector<VulkanShader>& shaders,
    VulkanPipeline& pipeline
) {
    vector<VkDescriptorPoolSize> sizes;

    for (auto& shader: shaders) {
        for (auto& set: shader.sets) {
            for (uint32_t i = set->set; i < set->binding_count; i++) {
                auto& spirv = *(set->bindings[i]);
                auto type = (VkDescriptorType)spirv.descriptor_type;

                VkDescriptorPoolSize* size = nullptr;
                for (auto& candidate: sizes) {
                    if (candidate.type == type) {
                        size = &candidate;
                        break;
                    }
                }

                if (size == nullptr) {
                    auto& size = sizes.emplace_back();
                    size.descriptorCount = spirv.count;
                    size.type = type;
                } else {
                    size->descriptorCount++;
                }
            }
        }
    }

    if (sizes.size() == 0) {
        pipeline.descriptorPool = VK_NULL_HANDLE;
    } else {
        VkDescriptorPoolCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        createInfo.maxSets = 1;
        createInfo.poolSizeCount = sizes.size();
        createInfo.pPoolSizes = sizes.data();

        checkSuccess(vkCreateDescriptorPool(
            vk.device,
            &createInfo,
            nullptr,
            &pipeline.descriptorPool
        ));
    }
}

void allocateDescriptorSet(Vulkan& vk, VulkanPipeline& pipeline) {
    if (pipeline.descriptorPool == VK_NULL_HANDLE) {
        pipeline.descriptorSet = VK_NULL_HANDLE;
    } else {
        VkDescriptorSetAllocateInfo allocateInfo = {};
        allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocateInfo.descriptorPool = pipeline.descriptorPool;
        allocateInfo.descriptorSetCount = 1;
        allocateInfo.pSetLayouts = &pipeline.descriptorLayout;
        checkSuccess(vkAllocateDescriptorSets(
            vk.device,
            &allocateInfo,
            &pipeline.descriptorSet
        ));
    }
}

void createShaderModule(
    Vulkan& vk,
    const vector<char>& code,
    VulkanShader& shader
) {
    SpvReflectResult result = spvReflectCreateShaderModule(
        code.size(),
        code.data(),
        &shader.reflect
    );
    assert(result == SPV_REFLECT_RESULT_SUCCESS);
    
    uint32_t setCount = 0;
    spvReflectEnumerateDescriptorSets(&shader.reflect, &setCount, nullptr);
    shader.sets.resize(setCount);
    spvReflectEnumerateDescriptorSets(
        &shader.reflect,
        &setCount,
        shader.sets.data()
    );

    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
    checkSuccess(vkCreateShaderModule(
        vk.device,
        &createInfo,
        nullptr,
        &shader.module
    ));
}

void createShaderModule(Vulkan& vk, const string& path, VulkanShader& shader) {
    auto code = readFile(path);
    createShaderModule(vk, code, shader);
}

void createPipelineLayout(Vulkan& vk, VulkanPipeline& pipeline) {
    VkPipelineLayoutCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    createInfo.setLayoutCount = 1;
    createInfo.pSetLayouts = &pipeline.descriptorLayout;
    checkSuccess(vkCreatePipelineLayout(
        vk.device,
        &createInfo,
        nullptr,
        &pipeline.layout
    ));
}

bool compareInputAttributes(
    SpvReflectInterfaceVariable* lhs,
    SpvReflectInterfaceVariable* rhs
) {
    return lhs->location < rhs->location;
}

void describeInputAttributes(
    VulkanPipeline& pipeline,
    VulkanShader& shader
) {
    uint32_t count = 0;
    spvReflectEnumerateInputVariables(&shader.reflect, &count, nullptr);
    vector<SpvReflectInterfaceVariable*> inputs(count);
    spvReflectEnumerateInputVariables(&shader.reflect, &count, inputs.data());

    // NOTE(jan): input attributes may be enumerated out of order, so
    // they need to be sorted when calculating the offset
    std::sort(inputs.begin(), inputs.end(), compareInputAttributes);

    pipeline.inputBinding.stride = 0;
    for (auto input: inputs) {
        if (strcmp("inUV", input->name) == 0) {
            pipeline.needsTexCoords = true;
        } else if (strcmp("inNormal", input->name) == 0) {
            pipeline.needsNormals = true;
        } else if (strcmp("inColor", input->name) == 0) {
            pipeline.needsColor = true;
        }

        auto& desc = pipeline.inputAttributes.emplace_back();
        desc.binding = 0;
        desc.location = input->location;
        desc.format = (VkFormat)input->format;
        desc.offset = pipeline.inputBinding.stride;
        auto componentCount = max(
            input->numeric.vector.component_count,
            1
        );
        pipeline.inputBinding.stride +=
            input->numeric.scalar.width / 8 *
            componentCount;
    }
}

void createPipeline(
    Vulkan& vk,
    VulkanShader& vert,
    VulkanShader& frag,
    VulkanPipeline& pipeline
) {
    vector<VkPipelineShaderStageCreateInfo> shaderStages;
    if (vert.module) {
        VkPipelineShaderStageCreateInfo vertStage = {};
        vertStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertStage.module = vert.module;
        vertStage.pName = "main";
        shaderStages.push_back(vertStage);
    }
    if (frag.module) {
        VkPipelineShaderStageCreateInfo fragStage = {};
        fragStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragStage.module = frag.module;
        fragStage.pName = "main";
        shaderStages.push_back(fragStage);
    }

    pipeline.inputBinding.binding = 0;
    pipeline.inputBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    describeInputAttributes(
        pipeline,
        vert
    );

    VkPipelineVertexInputStateCreateInfo vertexInput = {};
    vertexInput.sType =
        VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInput.vertexBindingDescriptionCount = 1;
    vertexInput.pVertexBindingDescriptions = &pipeline.inputBinding;
    vertexInput.vertexAttributeDescriptionCount =
        (uint32_t)pipeline.inputAttributes.size();
    vertexInput.pVertexAttributeDescriptions = pipeline.inputAttributes.data();
    
    VkPipelineInputAssemblyStateCreateInfo assembly = {};
    assembly.sType =
        VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    assembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport = {};
    viewport.height = (float)vk.swap.extent.height;
    viewport.width = (float)vk.swap.extent.width;
    viewport.minDepth = 0.f;
    viewport.maxDepth = 1.f; 
    viewport.x = 0.f;
    viewport.y = 0.f;

    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent = vk.swap.extent;

    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo raster = {};
    raster.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    raster.frontFace = VK_FRONT_FACE_CLOCKWISE;
    raster.cullMode = VK_CULL_MODE_BACK_BIT;
    raster.lineWidth = 1.f;
    raster.polygonMode = VK_POLYGON_MODE_FILL;
    raster.rasterizerDiscardEnable = VK_FALSE;
    raster.depthClampEnable = VK_FALSE;
    raster.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo msample = {};
    msample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    msample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    msample.sampleShadingEnable = VK_FALSE;
    msample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    msample.minSampleShading = 1.0f;
    msample.pSampleMask = nullptr;
    msample.alphaToCoverageEnable = VK_FALSE;
    msample.alphaToOneEnable = VK_FALSE;

    VkPipelineDepthStencilStateCreateInfo depthStencilCreateInfo = {};
    depthStencilCreateInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencilCreateInfo.depthTestEnable = VK_TRUE;
    depthStencilCreateInfo.depthWriteEnable = VK_TRUE;
    depthStencilCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS;
    // TODO(jan): Experiment with enabling this for better performance.
    depthStencilCreateInfo.depthBoundsTestEnable = VK_FALSE;
    depthStencilCreateInfo.stencilTestEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState colorBlend = {};
    colorBlend.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                                VK_COLOR_COMPONENT_G_BIT |
                                VK_COLOR_COMPONENT_B_BIT |
                                VK_COLOR_COMPONENT_A_BIT;
    colorBlend.blendEnable = VK_FALSE;
    colorBlend.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlend.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlend.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlend.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlend.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlend.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo blending = {};
    blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    blending.logicOpEnable = VK_FALSE;
    blending.logicOp = VK_LOGIC_OP_COPY;
    blending.attachmentCount = 1;
    blending.pAttachments = &colorBlend;
    blending.blendConstants[0] = 0.0f;
    blending.blendConstants[1] = 0.0f;
    blending.blendConstants[2] = 0.0f;
    blending.blendConstants[3] = 0.0f;

    VkGraphicsPipelineCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    createInfo.stageCount = (uint32_t)shaderStages.size();
    createInfo.pStages = shaderStages.data();
    createInfo.pVertexInputState = &vertexInput;
    createInfo.pInputAssemblyState = &assembly;
    createInfo.pViewportState = &viewportState;
    createInfo.pRasterizationState = &raster;
    createInfo.pMultisampleState = &msample;
    createInfo.pColorBlendState = &blending;
    createInfo.pDepthStencilState = &depthStencilCreateInfo;
    createInfo.renderPass = vk.renderPass;
    createInfo.layout = pipeline.layout;
    createInfo.subpass = 0;
    
    checkSuccess(vkCreateGraphicsPipelines(
        vk.device,
        VK_NULL_HANDLE,
        1,
        &createInfo,
        nullptr,
        &pipeline.handle
    ));
}

void initVKPipeline(Vulkan& vk, char* name, VulkanPipeline& pipeline) {
    pipeline = {};

    vector<VulkanShader> shaders(2);

    char vertFile[255];
    sprintf_s(vertFile, "shaders/%s.vert.spv", name);
    if (_access_s(vertFile, 2) == 0) {
        createShaderModule(vk, vertFile, shaders[0]);
    }

    char fragFile[255];
    sprintf_s(fragFile, "shaders/%s.frag.spv", name);
    if (_access_s(vertFile, 2) == 0) {
        createShaderModule(vk, fragFile, shaders[1]);
    }

    createDescriptorLayout(vk, shaders, pipeline);
    createDescriptorPool(vk, shaders, pipeline);
    allocateDescriptorSet(vk, pipeline);
    createPipelineLayout(vk, pipeline);
    createPipeline(vk, shaders[0], shaders[1], pipeline);
}
