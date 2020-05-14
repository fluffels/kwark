#include <stdexcept>

#include "VulkanUtils.h"

uint32_t selectMemoryTypeIndex(
    VkPhysicalDeviceMemoryProperties& memories,
    VkMemoryRequirements requirements,
    VkMemoryPropertyFlags extraFlags
) {
    uint32_t typeIndex = 0;
    bool found = false;
    for (; typeIndex < memories.memoryTypeCount; typeIndex++) {
        if (requirements.memoryTypeBits & (1 << typeIndex)) {
            auto flags = memories.memoryTypes[typeIndex].propertyFlags;
            flags &= extraFlags; 
            if (flags == extraFlags) {
                found = true;
                break;
            }
        }
    }
    if (!found) {
        throw std::runtime_error("could not find memory type");
    }
    return typeIndex;
}
