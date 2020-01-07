#include "VulkanApplication.h"

VulkanApplication::
VulkanApplication() {
    initVkInstance();
}

VulkanApplication::
~VulkanApplication() {

}

void VulkanApplication::
initVkInstance() {
    vkEnumerateInstanceVersion(&_versionApi);
    logVersion(_versionApi);
}

void VulkanApplication::
logVersion(uint32_t version) {
    LOG(INFO) << "Instance version: " << VK_VERSION_MAJOR(version) << "."
                                      << VK_VERSION_MINOR(version) << "."
                                      << VK_VERSION_PATCH(version);
}