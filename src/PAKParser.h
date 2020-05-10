#pragma once

/* See: http://www.gamers.org/dEngine/quake/spec/quake-spec34/qkspec_4.htm */

#include <iostream>
#include <string>
#include <vector>

#include "easylogging++.h"

#include "BSPParser.h"

using std::runtime_error;
using std::string;
using std::vector;

struct PAKHeader {
    char id[4];
    int32_t offset;
    int32_t size;
};

struct PAKFileEntry {
    char name[54];
    int32_t offset;
    int32_t size;
};

struct PAKParser {
    FILE* file;
    PAKHeader header;
    vector<PAKFileEntry> entries;

    BSPParser* map;

    PAKParser(const char*);
    ~PAKParser();
    void parseHeader();
    void parseEntries();

    BSPParser* loadMap(const string&);
};
