#include "Texture.h"

Atlas::Atlas(FILE* file, int32_t offset):
    file(file),
    baseOffset(offset)
{
    parseHeader();
    parseTextureHeaders();
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
