#pragma once

#include <array>

#include "vulkan/vulkan.h"

#include <glm/glm.hpp>

class Vertex {
    public:
        glm::vec3 pos;
        glm::vec3 normal;
        glm::vec3 light;

        static VkVertexInputBindingDescription
        getInputBindingDescription();

        static std::array<VkVertexInputAttributeDescription, 3>
        getInputAttributeDescriptions();
};
