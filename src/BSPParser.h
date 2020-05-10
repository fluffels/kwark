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

struct BSPParser {
    FILE* file;
    BSPHeader header;

    vector<Entity> entities;
    vector<Edge> edges;
    vector<vec3> lines;
    vector<vec3> vertices;
    
    BSPParser(FILE*, int32_t);

    void parseEdges(int32_t, int32_t);
    void parseEntities(int32_t, int32_t);
    void parseHeader(int32_t);
    void parseVertices(int32_t, int32_t);

    Entity& findEntityByName(char*);
};
