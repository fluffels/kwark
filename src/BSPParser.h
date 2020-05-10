#pragma once

#include <iostream>
#include <vector>

#include <glm/vec3.hpp>

using glm::vec3;

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

struct BSPParser {
    vector<vec3> lines;
    vec3 initEye;
    int initAngle;
    
    BSPParser(FILE* file, int32_t offset);
};
