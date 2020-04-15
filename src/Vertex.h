#pragma once

#include <array>

#include "vulkan.h"

#include <glm/glm.hpp>

class Vertex {
    public:
        glm::vec3 pos;

        static VkVertexInputBindingDescription
        getInputBindingDescription();

        static std::array<VkVertexInputAttributeDescription, 1>
        getInputAttributeDescriptions();
};
