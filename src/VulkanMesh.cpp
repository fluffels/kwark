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
