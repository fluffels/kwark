#include "util.h"

const char**
stringVectorToC(const vector<string>& v) {
    auto count = v.size();
    auto strings = new const char*[count];
    for (unsigned i = 0; i < count; i++) {
        strings[i] = v[i].c_str();
    }
    return strings;
}
