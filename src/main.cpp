#include "easylogging++.h"
INITIALIZE_EASYLOGGINGPP

#include "VulkanApplication.h"

int
main() {
    LOG(INFO) << "Initializing...";
    VulkanApplication vk;
    return 0;
}
