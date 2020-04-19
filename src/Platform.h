#pragma once

#include <string>
#include <vector>

#include <vulkan/vulkan.h>

using std::string;
using std::vector;

class Platform {
    public:
        virtual VkSurfaceKHR getSurface(const VkInstance&) const = 0;
        virtual vector<string> getExtensions() const = 0;
};
