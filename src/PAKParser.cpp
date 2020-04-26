#include <PAKParser.h>

#define readStruct(f, s) fread_s(&s, sizeof(s), sizeof(s), 1, f)

void seek(FILE* file, int32_t offset) {
    auto code = fseek(file, offset, SEEK_SET);
    if (code != 0) {
        throw std::runtime_error("could not seek to position");
    }
}

PAKHeader parsePAKHeader(FILE* file) {
    PAKHeader result = {};
    readStruct(file, result);
    if (strncmp("PACK", result.id, HEADER_LENGTH) != 0) {
        throw runtime_error("this is not a PAK file");
    }
    return result;
}

PAKFileEntry parsePAKFileEntry(FILE* file, int32_t offset, int idx) {
    PAKFileEntry result = {};
    seek(file, offset + idx * FILE_ENTRY_LENGTH);
    readStruct(file, result);
    return result;
}

BSPFile parseBSPFile(FILE* file, int32_t offset) {
    BSPFile result = {};
    seek(file, offset);
    readStruct(file, result);
    return result;
}

void parsePAK(const char* path) {
    FILE* file = nullptr;
    errno_t error = fopen_s(&file, path, "r");
    if (error != 0) {
        throw runtime_error("could not open PAK file");
    }

    auto header = parsePAKHeader(file);
    LOG(INFO) << "offset: " << header.offset;
    LOG(INFO) << "size: " << header.size;
    auto fileCount = header.size / FILE_ENTRY_LENGTH;
    LOG(INFO) << "contains " << fileCount << " files";

    for (int i = 0; i < fileCount; i++) {
        auto entry = parsePAKFileEntry(file, header.offset, i);
        LOG(INFO) << "file " << i << ": " << entry.name;

        if (strcmp("maps/e1m1.bsp", entry.name) == 0) {
            auto BSPFile = parseBSPFile(file, entry.offset);
        } else {
            seek(file, header.offset + (FILE_ENTRY_LENGTH*i));
        }
    }
}
