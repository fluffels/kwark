#pragma once

/* See: http://www.gamers.org/dEngine/quake/spec/quake-spec34/qkspec_4.htm */

#include <iostream>

#include <vector>

#include <glm/vec3.hpp>

#include "easylogging++.h"

using glm::vec3;

using std::runtime_error;
using std::vector;

struct PAKParser {
    vector<vec3> lines;
    vec3 initEye;
    float initAngle;
    
    PAKParser(const char*);
};
