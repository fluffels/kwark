#pragma once

#include <vector>

#include "BSPParser.h"
#include "Vertex.h"

using std::vector;

struct Mesh {
    BSPParser& bsp;
    vector<Vertex> vertices;

    Mesh(BSPParser& BSPParser);
    void buildWireFrameModel();
};
