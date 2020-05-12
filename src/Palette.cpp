#include "FileSystem.h"
#include "Palette.h"

Palette::Palette(FILE* file, int32_t offset, int32_t size) {
    seek(file, offset);
    colors.resize(size / 3);
    fread_s(colors.data(), size, size, 1, file);
}
