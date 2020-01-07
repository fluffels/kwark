#include "easylogging++.h"
INITIALIZE_EASYLOGGINGPP

#include "VulkanApplication.h"

using std::exception;

int
main() {
    LOG(INFO) << "Initializing...";
    try {
        VulkanApplication vk;
    } catch (exception e) {
        LOG(ERROR) << e.what();
    }
    return 0;
}
