#pragma once

#include <string>
#include <vector>

#include "easylogging++.h"

using std::string;
using std::vector;

#define checkSuccess(r) \
    if (r != VK_SUCCESS) { \
        LOG(ERROR) << __FILE__ << ":" << __LINE__; \
        exit(-1); \
    }

const char** stringVectorToC(const vector<string>&);
