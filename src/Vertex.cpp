#include "Vertex.h"

VkVertexInputBindingDescription Vertex::
getInputBindingDescription() {
    VkVertexInputBindingDescription i = {};
    i.binding = 0;
    i.stride = sizeof(Vertex);
    i.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    return i;
}

std::array<VkVertexInputAttributeDescription, 3> Vertex::
getInputAttributeDescriptions() {
    std::array<VkVertexInputAttributeDescription, 3> i = {};

    i[0].binding = 0;
    i[0].location = 0;
    i[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    i[0].offset = offsetof(Vertex, pos);

    i[1].binding = 0;
    i[1].location = 1;
    i[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    i[1].offset = offsetof(Vertex, color);

    i[2].binding = 0;
    i[2].location = 2;
    i[2].format = VK_FORMAT_R32G32B32_SFLOAT;
    i[2].offset = offsetof(Vertex, normal);

    return i;
}
