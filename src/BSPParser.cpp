#include "BSPParser.h"
#include "FileSystem.h"

using std::runtime_error;

void BSPParser::parseHeader() {
    seek(file, fileOffset);
    readStruct(file, header);
    if (header.version != 29) {
        throw runtime_error("BSP is not version 29");
    }
}

void BSPParser::parseFaces() {
    auto offset = fileOffset + header.faces.offset;
    auto size = header.faces.size;

    auto count = size / sizeof(Face);
    faces.resize(count);

    seek(file, offset);
    size_t readCount = fread_s(faces.data(), size, sizeof(Face), count, file);
    if (readCount != count) {
        throw runtime_error("unexpected eof");
    }
}

void parseOrigin(char *buffer, vec3 &origin) {
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

void BSPParser::parseEntities() {
    auto offset = fileOffset + header.entities.offset;
    auto size = header.entities.size;

    seek(file, offset);
    char* entityBuffer = new char[size];
    fread_s(entityBuffer, size, size, 1, file);
    char* ePos = entityBuffer;

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

    while(ePos < entityBuffer + size) {
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
                    entities.push_back(entity);
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
}

void BSPParser::parseVertices() {
    auto offset = fileOffset + header.vertices.offset;
    auto size = header.vertices.size;

    const int count = size / sizeof(vec3);
    vertices.resize(count);

    seek(file, offset);
    int32_t bytes = sizeof(vec3) * count;
    fread_s(vertices.data(), bytes, bytes, 1, file);

    /* NOTE(jan): Translate from BSP coordinate system.
       See: http://www.gamers.org/dEngine/quake/spec/quake-spec34/qkspec_2.htm#2.1.1 */
    for (vec3& vertex: vertices) {
        auto y = -vertex.z;
        auto z = -vertex.y;
        vertex.y = y;
        vertex.z = z;
    }
}

void BSPParser::parseEdges() {
    auto offset = fileOffset + header.edges.offset;
    auto size = header.edges.size;

    const int count = size / sizeof(Edge);
    edges.resize(count);

    seek(file, offset);
    int32_t bytes = sizeof(Edge) * count;
    fread_s(edges.data(), bytes, bytes, 1, file);
}

Entity& BSPParser::findEntityByName(char* name) {
    for (Entity& entity: entities) {
        if (strcmp(name, entity.className) == 0) {
            return entity;
        }
    }
    throw runtime_error("could not find entity " + string(name));
}

BSPParser::BSPParser(FILE* file, int32_t offset):
        file(file),
        fileOffset(offset) {
    parseHeader();
    parseEntities();
    parseVertices();
    parseEdges();
    parseFaces();

    for (int i = 0; i < edges.size(); i++) {
        auto edge = edges[i];
        if ((edge.v0 < vertices.size()) && (edge.v1 < vertices.size())) {
            lines.push_back(vertices[edge.v0]);
            lines.push_back(vertices[edge.v1]);
        }
    }
}
