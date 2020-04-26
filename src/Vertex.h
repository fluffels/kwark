#pragma once

#include <array>

#include "vulkan/vulkan.h"

#include <glm/glm.hpp>

class Vertex {
    public:
        glm::vec3 pos;
        glm::vec3 color;

        static VkVertexInputBindingDescription
        getInputBindingDescription();

        static std::array<VkVertexInputAttributeDescription, 2>
        getInputAttributeDescriptions();
};
