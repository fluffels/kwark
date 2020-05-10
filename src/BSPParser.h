#pragma once

#include <iostream>
#include <string>
#include <vector>

#include <glm/vec3.hpp>

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

struct BSPParser {
    FILE* file;
    int32_t fileOffset;
    BSPHeader header;

    vector<Entity> entities;
    vector<Edge> edges;
    vector<int32_t> edgeList;
    vector<Face> faces;
    vector<vec3> lines;
    vector<vec3> vertices;
    
    BSPParser(FILE*, int32_t);

    void parseEdges();
    void parseEdgeList();
    void parseEntities();
    void parseFaces();
    void parseHeader();
    void parseVertices();

    Entity& findEntityByName(char*);
};
