#pragma once

/* See: http://www.gamers.org/dEngine/quake/spec/quake-spec34/qkspec_4.htm */

#include <iostream>

#include <vector>

#include "easylogging++.h"

#include "BSPParser.h"

using std::runtime_error;
using std::vector;

struct PAKParser {
    BSPParser* map;

    PAKParser(const char*);
    ~PAKParser();
};
