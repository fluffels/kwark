#include "Vertex.h"

VkVertexInputBindingDescription Vertex::
getInputBindingDescription() {
    VkVertexInputBindingDescription i = {};
    i.binding = 0;
    i.stride = sizeof(Vertex);
    i.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    return i;
}

std::array<VkVertexInputAttributeDescription, 4> Vertex::
getInputAttributeDescriptions() {
    std::array<VkVertexInputAttributeDescription, 4> i = {};

    i[0].binding = 0;
    i[0].location = 0;
    i[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    i[0].offset = offsetof(Vertex, pos);

    i[1].binding = 0;
    i[1].location = 1;
    i[1].format = VK_FORMAT_R32G32_SFLOAT;
    i[1].offset = offsetof(Vertex, texCoord);

    i[2].binding = 0;
    i[2].location = 2;
    i[2].format = VK_FORMAT_R32_UINT;
    i[2].offset = offsetof(Vertex, texIdx);

    i[3].binding = 0;
    i[3].location = 3;
    i[3].format = VK_FORMAT_R32G32B32_SFLOAT;
    i[3].offset = offsetof(Vertex, light);

    return i;
}
