#pragma once

#include <array>

#include <vulkan/vulkan.h>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

class Vertex {
    public:
        glm::vec3 pos;
        glm::vec2 texCoord;
        uint32_t texIdx;
        glm::vec3 light;

        static VkVertexInputBindingDescription
        getInputBindingDescription();

        static std::array<VkVertexInputAttributeDescription, 4>
        getInputAttributeDescriptions();
};
