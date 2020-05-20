#include <cmath>

#include "BSPParser.h"
#include "FileSystem.h"

using std::runtime_error;

void fixCoords(vec3& v) {
    /* NOTE(jan): Translate from BSP coordinate system.
       See: http://www.gamers.org/dEngine/quake/spec/quake-spec34/qkspec_2.htm#2.1.1 */
    auto y = -v.z;
    auto z = -v.y;
    v.y = y;
    v.z = z;
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

Entity& BSPParser::findEntityByName(char* name) {
    for (Entity& entity: entities) {
        if (strcmp(name, entity.className) == 0) {
            return entity;
        }
    }
    throw runtime_error("could not find entity " + string(name));
}

void BSPParser::parseHeader() {
    seek(file, fileOffset);
    readStruct(file, header);
    if (header.version != 29) {
        throw runtime_error("BSP is not version 29");
    }
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

template<class T>
void BSPParser::parseLump(BSPEntry& entry, vector<T>& vec) {
    auto size = entry.size;
    auto count = size / sizeof(T);
    vec.resize(count);

    auto offset = fileOffset + entry.offset;
    seek(file, offset);
    size_t readCount = fread_s(vec.data(), size, sizeof(T), count, file);
}

BSPParser::BSPParser(FILE* file, int32_t offset, Palette& palette):
        atlas(nullptr),
        file(file),
        fileOffset(offset)
{
    parseHeader();

    atlas = new Atlas(file, fileOffset + header.miptex.offset, palette);

    parseLump(header.models, models);
    parseEntities();
    parseLump(header.vertices, vertices);
    parseLump(header.edges, edges);
    parseLump(header.ledges, edgeList);
    parseLump(header.faces, faces);
    parseLump(header.lightmaps, lightMap);
    parseLump(header.planes, planes);
    parseLump(header.texinfo, texInfos);

    for (vec3& vertex: vertices) {
        fixCoords(vertex);
    }
    for (auto& texInfo: texInfos) {
        fixCoords(texInfo.uVector);
        fixCoords(texInfo.vVector);
    }
}

BSPParser::~BSPParser() {
    delete atlas;
}
