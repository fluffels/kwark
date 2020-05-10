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

        if (strcmp("maps/e1m2.bsp", entry.name) == 0) {
            parseBSP(file, entry.offset);
        }
    }
}

void parseOrigin(char *buffer, Vec3 &origin) {
    char *s = strstr(buffer, " ");
    *s = '\0';
    origin.x = (float)atoi(buffer);

    char *n = s + 1;
    s = strstr(n, " ");
    *s = '\0';
    origin.z = (float)-atoi(n);

    n = s + 1;
    origin.y = (float)-atoi(n);
}

vector<Entity> parseEntities(FILE* file, int32_t offset, int32_t size) {
    vector<Entity> entityList;

    seek(file, offset);

    char* entities = new char[size];
    fread_s(entities, size, size, 1, file);
    char* ePos = entities;

    char buffer[255];
    char* bPos = buffer;

    enum KEY {
        CLASS_NAME,
        ORIGIN,
        ANGLE,
        UNKNOWN
    };
    KEY key = UNKNOWN;

    enum STATE {
        OUTSIDE_ENTITY,
        INSIDE_ENTITY,
        INSIDE_STRING,
    };
    STATE state = OUTSIDE_ENTITY;
    Entity entity;

    while(ePos < entities + size) {
        char c = *ePos;
        switch (state) {
            case OUTSIDE_ENTITY:
                if (c == '{') {
                    state = INSIDE_ENTITY;
                    entity = {};
                }
                ePos++;
                break;
            case INSIDE_ENTITY:
                if (c == '"') {
                    bPos = buffer;
                    state = INSIDE_STRING;
                } else if (*ePos == '}') {
                    state = OUTSIDE_ENTITY;
                    entityList.push_back(entity);
                }
                ePos++;
                break;
            case INSIDE_STRING:
                if (*ePos == '"') {
                    state = INSIDE_ENTITY;
                    *bPos = '\0';
                    switch (key) {
                        case CLASS_NAME:
                            strncpy_s(entity.className, buffer, 255);
                            break;
                        case ORIGIN:
                            parseOrigin(buffer, entity.origin);
                            break;
                        case ANGLE:
                            entity.angle = atoi(buffer);
                            break;
                        default:
                            break;
                    }
                    if (strcmp("classname", buffer) == 0) key = CLASS_NAME;
                    else if (strcmp("origin", buffer) == 0) key = ORIGIN;
                    else if (strcmp("angle", buffer) == 0) key = ANGLE;
                    else key = UNKNOWN;
                } else {
                    *bPos = c;
                    bPos++;
                }
                ePos++;
                break;
        }
    }
    return entityList;
}

void PAKParser::parseBSP(FILE* file, int32_t offset) {
    auto BSPHeader = parseBSPHeader(file, offset);
    auto entityList = parseEntities(
        file,
        offset + BSPHeader.entities.offset,
        BSPHeader.entities.size
    );

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

    seek(file, BSPHeader.vertices.offset + offset);
    const int vertexCount = BSPHeader.vertices.size / sizeof(Vec3);
    Vec3* vertices = new Vec3[vertexCount];
    fread_s(vertices, sizeof(Vec3)*vertexCount, sizeof(Vec3)*vertexCount, 1, file);

    seek(file, BSPHeader.edges.offset + offset);
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
}
