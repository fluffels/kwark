#include "Vertex.h"

VkVertexInputBindingDescription Vertex::
getInputBindingDescription() {
    VkVertexInputBindingDescription i = {};
    i.binding = 0;
    i.stride = sizeof(Vertex);
    i.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    return i;
}

std::array<VkVertexInputAttributeDescription, 2> Vertex::
getInputAttributeDescriptions() {
    std::array<VkVertexInputAttributeDescription, 2> i = {};

    i[0].binding = 0;
    i[0].location = 0;
    i[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    i[0].offset = offsetof(Vertex, pos);

    i[1].binding = 0;
    i[1].location = 1;
    i[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    i[1].offset = offsetof(Vertex, color);

    return i;
}
