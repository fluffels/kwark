#pragma once

#include <exception>
#include <map>
#include <vector>

#include <glm/vec3.hpp>

#include "FileSystem.h"
#include "Palette.h"

using glm::vec3;

using std::map;
using std::runtime_error;
using std::vector;

struct TextureIndex {
    int32_t numtex;
    vector<int32_t> offset;
};

struct TextureHeader {
    char name[16];
    uint32_t width;
    uint32_t height;
    uint32_t offset1;
    uint32_t offset2;
    uint32_t offset4;
    uint32_t offset8;
};

struct BSPTextureParser {
    vector<vector<uint8_t>> textures;
    vector<TextureHeader> textureHeaders;
    map<uint32_t, uint32_t> textureIDMap;

    BSPTextureParser(FILE*, int32_t, Palette&);

private:
    int32_t baseOffset;
    FILE* file;
    Palette& palette;

    TextureIndex header;

    void parseHeader();
    void parseTextureHeaders();
    void parseTexture(int, vector<uint8_t>&);
    void parseTextures();
};