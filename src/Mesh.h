#pragma once

#include <vector>

#include <glm/vec2.hpp>

#include "BSPParser.h"

using glm::ivec2;

using std::vector;

const int MAX_LIGHT_STYLES = 4;

struct Vertex {
    glm::vec3 pos;
    glm::vec2 texCoord;
    uint32_t texIdx;
    glm::vec2 lightCoord;
    int32_t lightIdx;
    int32_t lightStyles[MAX_LIGHT_STYLES];
    glm::vec2 extent;

    Vertex();
};

// TODO(jan): rename to "model" put textures, lightmaps, vertices &c in here
struct Mesh {
    BSPParser& bsp;
    vector<Vertex> vertices;
    vector<Vertex> skyVertices;
    vector<Vertex> fluidVertices;
    vector<float> lightMap;

    Mesh(BSPParser& BSPParser);
    void buildLightMap();
    void buildWireFrameModel();
};
