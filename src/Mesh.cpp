#include "Mesh.h"

Mesh::Mesh(BSPParser& bsp):
    bsp(bsp)
{
    buildWireFrameModel();
}

void Mesh::buildWireFrameModel() {
    for (Face& face: bsp.faces) {
        vector<vec3> faceVertices;
        auto edgeListBaseId = face.ledgeId;
        for (uint32_t i = 0; i < face.ledgeNum; i++) {
            auto edgeListId = edgeListBaseId + i;
            auto edgeId = bsp.edgeList[edgeListId];
            Edge& edge = bsp.edges[abs(edgeId)];
            vec3 v0 = bsp.vertices[edge.v0];
            vec3 v1 = bsp.vertices[edge.v1];
            if (edgeId < 0) {
                faceVertices.push_back(v1);
                faceVertices.push_back(v0);
            } else if (edgeId > 0) {
                faceVertices.push_back(v0);
                faceVertices.push_back(v1);
            }
        }

        auto light = 1.0f - face.baseLight / 255.0f;
        if (face.lightmap != -1) {
        }

        auto& plane = bsp.planes[face.planeId];
        auto& normal = plane.normal;

        Vertex v0, v1, v2;
        v0.color = { 0, light, 1 };
        v1.color = { 0, light, 1 };
        v2.color = { 0, light, 1 };
        v0.normal = normal;
        v1.normal = normal;
        v2.normal = normal;

        auto& p0 = faceVertices[0];
        v0.pos = p0;

        for (uint32_t i = 1; i < face.ledgeNum; i++) {
            v1.pos = faceVertices[i*2];
            v2.pos = faceVertices[i*2+1];

            vertices.push_back(v0);
            vertices.push_back(v1);
            // vertices.push_back(v1);
            vertices.push_back(v2);
            // vertices.push_back(v2);
            // vertices.push_back(v0);
        }
    }
}
