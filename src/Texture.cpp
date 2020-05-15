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
    auto& header = textureHeaders[0];
    auto size = header.width * header.height;

    textureColorIndices.resize(size);

    seek(file, baseOffset + header.offset1);
    fread_s(textureColorIndices.data(), size, size, 1, file);

    texture.resize(size);

    for (uint32_t i = 0; i < size; i++) {
        auto colorIdx = textureColorIndices[i];
        auto paletteColor = palette.colors[colorIdx];
        texture[i].r = paletteColor.r / 255.f;
        texture[i].g = paletteColor.g / 255.f;
        texture[i].b = paletteColor.b / 255.f;
    }
}
