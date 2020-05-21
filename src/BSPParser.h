#pragma once

#include <iostream>
#include <string>
#include <vector>

#include <glm/vec3.hpp>

#include "BSPTextureParser.h"
#include "Palette.h"

using glm::vec3;

using std::string;
using std::vector;

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

struct BoundingBox {
    vec3 min;
    vec3 max;
};

struct Entity {
    char className[255];
    vec3 origin;
    int angle;
};

struct Edge {
    uint16_t v0;
    uint16_t v1;
};

struct Face {
    uint16_t planeId;
    uint16_t side;
    int32_t ledgeId;
    uint16_t ledgeNum;
    uint16_t texinfoId;
    uint8_t typeLight;
    uint8_t baseLight;
    uint8_t light[2];
    int32_t lightmap;
};

struct Model {
    BoundingBox bounds;
    vec3 origin;
    int32_t bsp;
    int32_t clip1;
    int32_t clip2;
    int32_t node3;
    int32_t leafCount;
    int32_t faceID;
    int32_t faceCount;
};

struct Plane {
    vec3 normal;
    float dist;
    int32_t type;
};

struct TexInfo {
    vec3 uVector;
    float uOffset;
    vec3 vVector;
    float vOffset;
    uint32_t textureID;
    uint32_t animated;
};

struct BSPParser {
    BSPHeader header;

    BSPTextureParser* textures;

    vector<Entity> entities;
    vector<Edge> edges;
    vector<int32_t> edgeList;
    vector<Face> faces;
    vector<vec3> lines;
    vector<uint8_t> lightMap;
    vector<Model> models;
    vector<Plane> planes;
    vector<TexInfo> texInfos;
    vector<vec3> vertices;
    
    BSPParser(FILE*, int32_t, Palette&);
    ~BSPParser();

    Entity& findEntityByName(char*);

private:
    FILE* file;
    int32_t fileOffset;

    void parseEntities();
    void parseHeader();
    template<class T> void parseLump(BSPEntry& entry, vector<T>& vec);
};
