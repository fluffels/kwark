#include "Vertex.h"

VkVertexInputBindingDescription Vertex::
getInputBindingDescription() {
    VkVertexInputBindingDescription i = {};
    i.binding = 0;
    i.stride = sizeof(Vertex);
    i.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    return i;
}

std::array<VkVertexInputAttributeDescription, 1> Vertex::
getInputAttributeDescriptions() {
    std::array<VkVertexInputAttributeDescription, 1> i = {};
    i[0].binding = 0;
    i[0].location = 0;
    i[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    i[0].offset = offsetof(Vertex, pos);
    return i;
}
