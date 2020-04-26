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

BSPHeader parseBSPHeader(FILE* file, int32_t offset) {
    BSPHeader result = {};
    seek(file, offset);
    readStruct(file, result);
    if (result.version != 29) {
        throw runtime_error("BSP is not version 29");
    }
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
            auto BSPHeader = parseBSPHeader(file, entry.offset);

            seek(file, BSPHeader.vertices.offset + entry.offset);
            const int vertexCount = BSPHeader.vertices.size / sizeof(Vec3);
            Vec3* vertices = new Vec3[vertexCount];
            fread_s(vertices, sizeof(Vec3)*vertexCount, sizeof(Vec3)*vertexCount, 1, file);
            delete vertices;

            seek(file, BSPHeader.edges.offset + entry.offset);
            const int edgeCount = BSPHeader.edges.size / sizeof(Edge);
            Edge* edges = new Edge[edgeCount];
            fread_s(edges, sizeof(Edge)*edgeCount, sizeof(Edge)*edgeCount, 1, file);
            delete edges;
        } else {
            seek(file, header.offset + (FILE_ENTRY_LENGTH*i));
        }
    }
}
