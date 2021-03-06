#include <glm/geometric.hpp>
#include <glm/vec2.hpp>

#include "Mesh.h"

using glm::dot;
using glm::normalize;
using glm::vec2;

Vertex::Vertex() {
    pos = {};
    texCoord = {};
    texIdx = -1;
    lightCoord = {};
    lightIdx = -1;
    extent = {};

    for (int i = 0; i < MAX_LIGHT_STYLES; i++) {
        lightStyles[i] = 0xFF;
    }
}

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
    buildLightMap();
    buildWireFrameModel();
}

void Mesh::buildLightMap() {
    auto count = bsp.lightMap.size();
    lightMap.resize(count);

    for (unsigned i = 0; i < count; i++) {
        lightMap[i] = bsp.lightMap[i] / 255.f;
    }
}

void Mesh::buildWireFrameModel() {
    for (auto& model: bsp.models) {
        auto firstFace = model.faceID;
        auto lastFace = firstFace + model.faceCount;

        for (int faceIdx = firstFace; faceIdx < lastFace; faceIdx++) {
            auto& face = bsp.faces[faceIdx];

            auto& texInfo = bsp.texInfos[face.texinfoId];
            auto& texID = texInfo.textureID;
            auto texType = bsp.textures->texTypes[texID];
            if (texType == TEXTYPE::DEBUG) {
                continue;
            }
            auto& texHeader = bsp.textures->textureHeaders[texInfo.textureID];

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

            auto texNum = bsp.textures->texNums[texID];
            v0.texIdx = texNum;
            v1.texIdx = texNum;
            v2.texIdx = texNum;

            auto& p0 = faceCoords[0];
            v0.pos = p0;
            v0.texCoord = calculateUV(v0.pos, texInfo);
            v0.lightCoord = {0, 0};
            v0.lightIdx = -1;

            vector<Vertex> faceVertices;
            for (uint32_t i = 1; i < face.ledgeNum; i++) {
                faceVertices.push_back(v0);

                v1.pos = faceCoords[i*2];
                v1.texCoord = calculateUV(v1.pos, texInfo);
                v1.lightCoord = {0, 0};
                v1.lightIdx = -1;
                faceVertices.push_back(v1);

                v2.pos = faceCoords[i*2+1];
                v2.texCoord = calculateUV(v2.pos, texInfo);
                v2.lightCoord = {0, 0};
                v2.lightIdx = -1;
                faceVertices.push_back(v2);
            }

            for (auto& vertex: faceVertices) {
                vertex.lightStyles[0] = face.typeLight;
                vertex.lightStyles[1] = face.baseLight;
                vertex.lightStyles[2] = face.light[0];
                vertex.lightStyles[3] = face.light[1];
            }

            if (face.lightmap != -1) {
                vec2 uvMin = faceVertices[0].texCoord;
                vec2 uvMax = faceVertices[0].texCoord;
                for (auto& v: faceVertices) {
                    auto& uv = v.texCoord;
                    if (uv.x < uvMin.x) uvMin.x = uv.x;
                    if (uv.y < uvMin.y) uvMin.y = uv.y;
                    if (uv.x > uvMax.x) uvMax.x = uv.x;
                    if (uv.y > uvMax.y) uvMax.y = uv.y;
                }
                // NOTE(jan): gl_model.c:1049--1050
                uvMin.x = floor(uvMin.x / 16);
                uvMin.y = floor(uvMin.y / 16);
                uvMax.x = ceil(uvMax.x / 16);
                uvMax.y = ceil(uvMax.y / 16);

                vec2 extent = {
                    int(uvMax.x - uvMin.x) + 1,
                    int(uvMax.y - uvMin.y) + 1
                };

                for (auto& v: faceVertices) {
                    auto& uv = v.texCoord;
                    v.lightCoord.s = (uv.x/16) - uvMin.x;
                    v.lightCoord.t = (uv.y/16) - uvMin.y;
                    v.lightIdx = face.lightmap;
                    v.extent = extent;
                }
            }

            for (auto& v: faceVertices) {
                auto& uv = v.texCoord;
                uv.x /= texHeader.width;
                uv.y /= texHeader.height;

                if (texType == TEXTYPE::SKY) {
                    skyVertices.push_back(v);
                } else if (texType == TEXTYPE::FLUID) {
                    fluidVertices.push_back(v);
                } else {
                    vertices.push_back(v);
                }
            }
        }
    }
}
