#include <glm/geometric.hpp>
#include <glm/vec2.hpp>

#include "Mesh.h"

using glm::dot;
using glm::normalize;
using glm::vec2;

vec2 calculateTexCoord(
    vec3& vertex,
    TexInfo& texInfo,
    TextureHeader& texHeader
) {
    vec2 result = {
        (dot(vertex, texInfo.uVector) + texInfo.uOffset) / texHeader.width,
        (dot(vertex, texInfo.vVector) + texInfo.vOffset) / texHeader.height
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

        auto light = 1.f - face.baseLight / 255.0f;
        if (face.lightmap != -1) {
            // TODO(jan): calculate light values from light map and send to shaders
        }

        auto& texInfo = bsp.texInfos[face.texinfoId];
        auto& texHeader = bsp.atlas->textureHeaders[texInfo.textureID];

        Vertex v0, v1, v2;
        v0.light = { light, light, light };
        v1.light = { light, light, light };
        v2.light = { light, light, light };

        auto texNum = bsp.atlas->textureIDMap[texInfo.textureID];
        v0.texIdx = texNum;
        v1.texIdx = texNum;
        v2.texIdx = texNum;

        auto& p0 = faceVertices[0];
        v0.pos = p0;
        v0.texCoord = calculateTexCoord(v0.pos, texInfo, texHeader);

        for (uint32_t i = 1; i < face.ledgeNum; i++) {
            vertices.push_back(v0);

            v1.pos = faceVertices[i*2];
            v1.texCoord = calculateTexCoord(v1.pos, texInfo, texHeader);
            vertices.push_back(v1);

            v2.pos = faceVertices[i*2+1];
            v2.texCoord = calculateTexCoord(v2.pos, texInfo, texHeader);
            vertices.push_back(v2);
        }
    }
}
