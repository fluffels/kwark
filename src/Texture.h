#pragma once

#include <exception>
#include <vector>

#include <glm/vec3.hpp>

#include "FileSystem.h"
#include "Palette.h"

using glm::vec3;

using std::runtime_error;
using std::vector;

struct AtlasHeader {
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

struct Atlas {
    FILE* file;
    int32_t baseOffset;

    AtlasHeader header;
    vector<TextureHeader> textureHeaders;
    vector<uint8_t> textureColorIndices;
    vector<vec3> texture;

    Atlas(FILE*, int32_t, Palette&);

    void parseHeader();
    void parseTextureHeaders();
    void parseTexture(Palette& palette);
};
