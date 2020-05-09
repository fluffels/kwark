#include <PAKParser.h>

#define readStruct(f, s) fread_s(&s, sizeof(s), sizeof(s), 1, f)

#define HEADER_LENGTH 4
#define FILE_ENTRY_LENGTH 64

struct PAKHeader {
    char id[4];
    int32_t offset;
    int32_t size;
};

struct PAKFileEntry {
    char name[54];
    int32_t offset;
    int32_t size;
};

struct BSPEntry {
    int32_t offset;
    int32_t size;
};

struct BSPHeader {
    int32_t version;
    BSPEntry entities;
    BSPEntry planes;
    BSPEntry miptex;
    BSPEntry vertices;
    BSPEntry visilist;
    BSPEntry nodes;
    BSPEntry texinfo;
    BSPEntry faces;
    BSPEntry lightmaps;
    BSPEntry clipnodes;
    BSPEntry leaves;
    BSPEntry lface;
    BSPEntry edges;
    BSPEntry ledges;
    BSPEntry models;
};

struct Vec3 {
    float x;
    float y;
    float z;
};

struct Entity {
    char className[255];
    Vec3 origin;
    int angle;
};

struct Edge {
    uint16_t v0;
    uint16_t v1;
};

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

PAKParser::PAKParser(const char* path) {
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

            vector<Entity> entityList;
            seek(file, BSPHeader.entities.offset + entry.offset);
            char* entities = new char[BSPHeader.entities.size];
            fread_s(entities, BSPHeader.entities.size, BSPHeader.entities.size, 1, file);
            char buffer[255];
            char* ePos = entities;
            char* bPos = buffer;
            bool inClassName = false;
            bool inOrigin = false;
            bool inAngle = false;
            int state = 0;
            Entity entity;
            while(ePos < entities + BSPHeader.entities.size) {
                if (state == 0) {
                    if (*ePos == '{') {
                        state = 1;
                    }
                    ePos++;
                    continue;
                } else if (state == 1) {
                    if (*ePos == '"') {
                        bPos = buffer;
                        state = 2;
                    } else if (*ePos == '}') {
                        entityList.push_back(entity);
                        entity = {};
                        state = 0;
                    }
                    ePos++;
                    continue;
                } else if (state == 2) {
                    if (*ePos == '"') {
                        *bPos = '\0';
                        if (inClassName) {
                            strcpy(entity.className, buffer);
                        } else if (inOrigin) {
                            char* s = strstr(buffer, " ");
                            *s = '\0';
                            entity.origin.x = atoi(buffer);

                            char* n = s + 1;
                            s = strstr(n, " ");
                            *s = '\0';
                            entity.origin.z = atoi(n);

                            n = s + 1;
                            entity.origin.y = atoi(n);
                        } else if (inAngle) {
                            entity.angle = atoi(buffer);
                        }
                        if (strcmp("classname", buffer) == 0) {
                            inClassName = true;
                        } else if (strcmp("origin", buffer) == 0) {
                            inOrigin = true;
                        } else if (strcmp("angle", buffer) == 0) {
                            inAngle = true;
                        } else {
                            inClassName = false;
                            inOrigin = false;
                            inAngle = false;
                        }
                        state = 1;
                    } else {
                        *bPos = *ePos;
                        bPos++;
                    }
                    ePos++;
                    continue;
                }
            }

            for (auto entity: entityList) {
                if (strcmp("info_player_start", entity.className) == 0) {
                    initEye = {
                        entity.origin.x,
                        entity.origin.y,
                        entity.origin.z
                    };
                    initAngle = entity.angle;
                }
            }

            seek(file, BSPHeader.vertices.offset + entry.offset);
            const int vertexCount = BSPHeader.vertices.size / sizeof(Vec3);
            Vec3* vertices = new Vec3[vertexCount];
            fread_s(vertices, sizeof(Vec3)*vertexCount, sizeof(Vec3)*vertexCount, 1, file);

            seek(file, BSPHeader.edges.offset + entry.offset);
            const int edgeCount = BSPHeader.edges.size / sizeof(Edge);
            Edge* edges = new Edge[edgeCount];
            fread_s(edges, sizeof(Edge)*edgeCount, sizeof(Edge)*edgeCount, 1, file);

            for (int i = 0; i < edgeCount; i++) {
                auto edge = edges[i];
                if ((edge.v0 < vertexCount) && (edge.v1 < vertexCount)) {
                    auto v = vertices[edge.v0];
                    lines.push_back(vec3(v.x, -v.z, -v.y));
                    v = vertices[edge.v1];
                    lines.push_back(vec3(v.x, -v.z, -v.y));
                }
            }

            delete vertices;
            delete edges;
        } else {
            seek(file, header.offset + (FILE_ENTRY_LENGTH*i));
        }
    }
}
