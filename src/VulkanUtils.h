#include <vulkan/vulkan.h>

uint32_t selectMemoryTypeIndex(
    VkPhysicalDeviceMemoryProperties&,
    VkMemoryRequirements,
    VkMemoryPropertyFlags
);
