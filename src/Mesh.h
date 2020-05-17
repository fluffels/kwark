#pragma once

#include <vector>

#include <glm/vec2.hpp>

#include "BSPParser.h"
#include "Vertex.h"

using glm::ivec2;

using std::vector;

// TODO(jan): rename to "model" put textures, lightmaps, vertices &c in here
struct Mesh {
    BSPParser& bsp;
    vector<Vertex> vertices;
    vector<ivec2> lightMapExtents;
    vector<vector<uint8_t>> lightMaps;

    Mesh(BSPParser& BSPParser);
    void buildWireFrameModel();
};
