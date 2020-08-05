#include <vulkan/vulkan.h>

VkPhysicalDeviceMemoryProperties getMemories(VkPhysicalDevice gpu);
uint32_t selectMemoryTypeIndex(
    VkPhysicalDeviceMemoryProperties&,
    VkMemoryRequirements,
    VkMemoryPropertyFlags
);
VkMemoryRequirements getMemoryRequirements(VkDevice, VkBuffer);
VkMemoryRequirements getMemoryRequirements(VkDevice, VkImage);
void* mapMemory(VkDevice, VkMemoryRequirements, VkDeviceMemory);
void* mapMemory(VkDevice, VkImage, VkDeviceMemory);
void* mapMemory(VkDevice, VkBuffer, VkDeviceMemory);
void unMapMemory(VkDevice, VkDeviceMemory);
