#pragma once

/*
See: http://www.gamers.org/dEngine/quake/spec/quake-spec34/qkspec_4.htm

Types mentioned in the link appear to be defined as follows:
  * long: int32_t
  * short: int32_t (odd, but ledges is marked as an array of short, but appears
      to be 4 byte integers in Quake's pak0 stored at offset 10777107)
  * u_short: uint16_t
*/

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

    PAKFileEntry& findEntry(const string&);

    BSPParser* loadMap(const string&);
};
