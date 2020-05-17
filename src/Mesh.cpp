#include <glm/geometric.hpp>
#include <glm/vec2.hpp>

#include "Mesh.h"

using glm::dot;
using glm::normalize;
using glm::vec2;

vec2 calculateUV(
    vec3& vertex,
    TexInfo& texInfo
) {
    vec2 result = {
        (dot(vertex, texInfo.uVector) + texInfo.uOffset),
        (dot(vertex, texInfo.vVector) + texInfo.vOffset)
    };
    return result;
}

Mesh::Mesh(BSPParser& bsp):
    bsp(bsp)
{
    buildWireFrameModel();
}

void Mesh::buildWireFrameModel() {
    for (Face& face: bsp.faces) {
        vector<vec3> faceCoords;
        auto edgeListBaseId = face.ledgeId;
        for (uint32_t i = 0; i < face.ledgeNum; i++) {
            auto edgeListId = edgeListBaseId + i;
            auto edgeId = bsp.edgeList[edgeListId];
            Edge& edge = bsp.edges[abs(edgeId)];
            vec3 v0 = bsp.vertices[edge.v0];
            vec3 v1 = bsp.vertices[edge.v1];
            if (edgeId < 0) {
                faceCoords.push_back(v1);
                faceCoords.push_back(v0);
            } else if (edgeId > 0) {
                faceCoords.push_back(v0);
                faceCoords.push_back(v1);
            }
        }

        Vertex v0, v1, v2;

        auto& texInfo = bsp.texInfos[face.texinfoId];
        auto& texHeader = bsp.atlas->textureHeaders[texInfo.textureID];

        auto texNum = bsp.atlas->textureIDMap[texInfo.textureID];
        v0.texIdx = texNum;
        v1.texIdx = texNum;
        v2.texIdx = texNum;

        auto& p0 = faceCoords[0];
        v0.pos = p0;
        v0.texCoord = calculateUV(v0.pos, texInfo);

        vector<Vertex> faceVertices;
        for (uint32_t i = 1; i < face.ledgeNum; i++) {
            faceVertices.push_back(v0);

            v1.pos = faceCoords[i*2];
            v1.texCoord = calculateUV(v1.pos, texInfo);
            faceVertices.push_back(v1);

            v2.pos = faceCoords[i*2+1];
            v2.texCoord = calculateUV(v2.pos, texInfo);
            faceVertices.push_back(v2);
        }

        vec2 uvMin = faceVertices[0].texCoord;
        vec2 uvMax = faceVertices[0].texCoord;
        for (auto& v: faceVertices) {
            auto& uv = v.texCoord;
            if (uv.x < uvMin.x) uvMin.x = uv.x;
            if (uv.y < uvMin.y) uvMin.y = uv.y;
            if (uv.x > uvMax.x) uvMax.x = uv.x;
            if (uv.y > uvMax.y) uvMax.y = uv.y;
        }
        auto texelWidth = int(uvMax.x - uvMin.x) / 16;
        auto texelHeight = int(uvMax.y - uvMin.y) / 16;

        auto baseLight = 1.f - face.baseLight / 255.f;
        for (auto& vertex: faceVertices) {
            if (face.lightmap >= 0) {
                auto& uv = vertex.texCoord;
                auto u = int(uv.x - uvMin.x) / 16;
                auto v = int(uv.y - uvMin.y) / 16;
                auto lightIdx = face.lightmap + v*texelWidth + u;
                auto lightMap = bsp.lightMap[lightIdx];
                vertex.light = baseLight + lightMap / 255.f;
            }
        }

        for (auto& v: faceVertices) {
            auto& uv = v.texCoord;
            uv.x /= texHeader.width;
            uv.y /= texHeader.height;
        }

        for (auto& v: faceVertices) {
            vertices.push_back(v);
        }
    }
}
