#include "Vulkan.h"

void uploadMesh(
    VkDevice device,
    VkPhysicalDeviceMemoryProperties& memories,
    uint32_t queueFamily,
    void* data,
    uint32_t size,
    VulkanMesh& mesh
) {
    createVertexBuffer(
        device,
        memories,
        queueFamily,
        size,
        mesh.vBuff
    );

    void* memory = mapMemory(device, mesh.vBuff.handle, mesh.vBuff.memory);
        memcpy(memory, data, size);
    unMapMemory(device, mesh.vBuff.memory);
}

void uploadMesh(
    VkDevice device,
    VkPhysicalDeviceMemoryProperties& memories,
    uint32_t queueFamily,
    void* vertices,
    uint32_t verticesSize,
    void* indices,
    uint32_t indicesSize,
    VulkanMesh& mesh
) {
    createVertexBuffer(
        device,
        memories,
        queueFamily,
        verticesSize,
        mesh.vBuff
    );

    void* memory = mapMemory(device, mesh.vBuff.handle, mesh.vBuff.memory);
        memcpy(memory, vertices, verticesSize);
    unMapMemory(device, mesh.vBuff.memory);

    createIndexBuffer(
        device,
        memories,
        queueFamily,
        indicesSize,
        mesh.iBuff
    );

    memory = mapMemory(device, mesh.iBuff.handle, mesh.iBuff.memory);
        memcpy(memory, indices, indicesSize);
    unMapMemory(device, mesh.iBuff.memory);
}
