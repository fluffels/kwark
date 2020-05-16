#include "Texture.h"

Atlas::Atlas(FILE* file, int32_t offset, Palette& palette):
    file(file),
    baseOffset(offset)
{
    parseHeader();
    parseTextureHeaders();
    parseTexture(palette);
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

void Atlas::parseTexture(Palette& palette) {
    auto headerOffset = header.offset[0];
    auto& header = textureHeaders[0];
    auto size = header.width * header.height;

    textureColorIndices.resize(size);

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
