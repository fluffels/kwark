#pragma once

#include <iostream>
#include <string>
#include <vector>

#include <glm/vec3.hpp>

#include "Texture.h"

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

struct Plane {
    vec3 normal;
    float dist;
    int32_t type;
};

struct BSPParser {
    FILE* file;
    int32_t fileOffset;
    BSPHeader header;

    Atlas* atlas;
    vector<Entity> entities;
    vector<Edge> edges;
    vector<int32_t> edgeList;
    vector<Face> faces;
    vector<vec3> lines;
    vector<uint8_t> lightMap;
    vector<Plane> planes;
    vector<vec3> vertices;
    
    BSPParser(FILE*, int32_t);
    ~BSPParser();

    void parseEdges();
    void parseEdgeList();
    void parseEntities();
    void parseFaces();
    void parseHeader();
    void parseLightMap();
    void parsePlanes();
    void parseVertices();

    void buildWireFrameModel();

    Entity& findEntityByName(char*);
};
