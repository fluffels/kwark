#include "BSPParser.h"
#include "FileSystem.h"

using std::runtime_error;

BSPHeader parseBSPHeader(FILE* file, int32_t offset) {
    BSPHeader result = {};
    seek(file, offset);
    readStruct(file, result);
    if (result.version != 29) {
        throw runtime_error("BSP is not version 29");
    }
    return result;
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

vector<Vec3> parseVertices(FILE* file, int32_t offset, int32_t size) {
    const int count = size / sizeof(Vec3);
    vector<Vec3> vertices(count);

    seek(file, offset);
    int32_t bytes = sizeof(Vec3) * count;
    fread_s(vertices.data(), bytes, bytes, 1, file);

    return vertices;
}

vector<Edge> parseEdges(FILE* file, int32_t offset, int32_t size) {
    const int count = size / sizeof(Edge);
    vector<Edge> edges(count);

    seek(file, offset);
    int32_t bytes = sizeof(Edge) * count;
    fread_s(edges.data(), bytes, bytes, 1, file);

    return edges;
}

BSPParser::BSPParser(FILE* file, int32_t offset) {
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

    auto vertices = parseVertices(
        file,
        offset + BSPHeader.vertices.offset,
        BSPHeader.vertices.size
    );
    auto edges = parseEdges(
        file,
        offset + BSPHeader.edges.offset,
        BSPHeader.edges.size
    );

    for (int i = 0; i < edges.size(); i++) {
        auto edge = edges[i];
        if ((edge.v0 < vertices.size()) && (edge.v1 < vertices.size())) {
            auto v = vertices[edge.v0];
            lines.push_back(vec3(v.x, -v.z, -v.y));
            v = vertices[edge.v1];
            lines.push_back(vec3(v.x, -v.z, -v.y));
        }
    }
}
