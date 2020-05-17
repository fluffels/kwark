#include "Texture.h"

Atlas::Atlas(FILE* file, int32_t offset, Palette& palette):
    baseOffset(offset),
    file(file),
    palette(palette)
{
    parseHeader();
    parseTextureHeaders();
    parseTextures();
}

void Atlas::parseHeader() {
    seek(file, baseOffset);
    readStruct(file, header.numtex);

    auto count = header.numtex;
    auto elementSize = sizeof(int32_t);

    header.offset.resize(count);
    auto readCount = fread_s(
        header.offset.data(),
        count * elementSize,
        elementSize,
        count,
        file
    );
    if (readCount != count) {
        throw runtime_error("unexpected EOF");
    }
}

void Atlas::parseTextureHeaders() {
    auto count = header.numtex;
    auto elementSize = sizeof(TextureHeader);

    textureHeaders.resize(count);

    for (int i = 0; i < count; i++) {
        auto& textureHeader = textureHeaders[i];

        auto texHeaderOffset = header.offset[i];
        if (texHeaderOffset > 0) {
            auto offset = baseOffset + texHeaderOffset;
            seek(file, offset);
            readStruct(file, textureHeader);
        } else {
            textureHeader = {};
        }
    }
}

void Atlas::parseTexture(int idx, vector<uint8_t>& texture) {
    auto headerOffset = header.offset[idx];
    auto& header = textureHeaders[idx];
    auto size = header.width * header.height;

    vector<uint8_t> textureColorIndices(size);

    seek(file, baseOffset + headerOffset + header.offset1);
    fread_s(textureColorIndices.data(), size, size, 1, file);

    texture.resize(size * 4);

    for (uint32_t i = 0; i < size; i++) {
        auto colorIdx = textureColorIndices[i];
        auto paletteColor = palette.colors[colorIdx];
        texture[i*4] = paletteColor.r;
        texture[i*4+1] = paletteColor.g;
        texture[i*4+2] = paletteColor.b;
        texture[i*4+3] = 255;
    }
}

void Atlas::parseTextures() {
    // NOTE(jan): for some reason, textures can sometimes have a zero area.
    // To prevent this causing problems we skip such textures and map
    // their indices to a large texture index so the error is obvious.
    for (int idx = 0; idx < header.numtex; idx++) {
        auto& textureHeader = textureHeaders[idx];
        if ((textureHeader.width > 0) && (textureHeader.height > 0)) {
            vector<uint8_t> texture = {};
            parseTexture(idx, texture);
            textures.push_back(texture);
            textureIDMap[idx] = (uint32_t)textures.size() - 1;
        } else {
            textureIDMap[idx] = -1;
        }
    }
}
