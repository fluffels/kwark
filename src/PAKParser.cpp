#include "FileSystem.h"
#include "Logging.h"
#include "PAKParser.h"

#define HEADER_LENGTH 4
#define FILE_ENTRY_LENGTH 64

void PAKParser::parseHeader() {
    readStruct(file, header);
    if (strncmp("PACK", header.id, HEADER_LENGTH) != 0) {
        throw runtime_error("this is not a PAK file");
    }
}

void PAKParser::parseEntries() {
    auto count = header.size / FILE_ENTRY_LENGTH;
    INFO("contains %d entries", count);
    entries.resize(count);

    seek(file, header.offset);
    size_t read = fread_s(
        entries.data(),
        sizeof(PAKFileEntry) * count,
        sizeof(PAKFileEntry),
        count,
        file
    );
    if (read != count) {
        throw runtime_error("unexpected EOF while reading PAK entries");
    }
}

PAKParser::PAKParser(const char* path):
        file(NULL),
        palette(nullptr) {
    errno_t error = fopen_s(&file, path, "rb");
    LERROR(error);

    parseHeader();
    parseEntries();
    palette = loadPalette();
}

PAKParser::~PAKParser() {
    fclose(file);
    delete palette;
}

PAKFileEntry& PAKParser::findEntry(const string& name) {
    for (auto& entry: entries) {
        if (strcmp(name.c_str(), entry.name) == 0) {
            return entry;
        }
    }
    throw std::runtime_error("could not find entity " + name);
}

BSPParser* PAKParser::loadMap(const string& name) {
    auto palette = loadPalette();
    string entryName = "maps/" + name + ".bsp";
    auto& entry = findEntry(entryName);
    return new BSPParser(file, entry.offset, *palette);
}

Palette* PAKParser::loadPalette() {
    auto& entry = findEntry("gfx/palette.lmp");
    return new Palette(file, entry.offset, entry.size);
}


