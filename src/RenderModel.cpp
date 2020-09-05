#pragma warning(disable: 4267)

#include "BSPParser.h"
#include "FileSystem.h"
#include "Model.h"
#include "Palette.h"
#include "PAKParser.h"
#include "Vulkan.h"

#include "glm/vec3.hpp"

typedef float scalar_t;
typedef glm::vec3 vec3_t;

// http://www.gamers.org/dEngine/quake/spec/quake-spec34/qkspec_5.htm
struct MDLHeader {
    char id[4];         // 0x4F504449 = "IDPO" for IDPOLYGON
    int32_t version;    // Version = 6
    vec3_t scale;    // Model scale factors.
    vec3_t origin;   // Model origin.
    scalar_t radius; // Model bounding radius.
    vec3_t offsets;  // Eye position (useless?)
    int32_t numskins;   // the number of skin textures
    int32_t skinwidth;  // Width of skin texture, multiple of 8
    int32_t skinheight; // Height of skin texture multiple of 8
    int32_t numverts;   // Number of vertices
    int32_t numtris;    // Number of triangles surfaces
    int32_t numframes;  // Number of frames
    int32_t synctype;   // 0= synchron, 1= random
    int32_t flags;      // 0 (see Alias models)
    scalar_t size;   // average size of triangles
};

struct TexCoord {
    int32_t onseam; // 0 or 0x20
    int32_t s;      // position, horizontally in range [0, skinwidth]
    int32_t t;      // position, vertically in range [0,skinheight]
};

struct Triangle {
    int32_t facesfront;  // boolean
    int32_t vertices[3]; // vertex indices [0, numverts]
};

struct FrameVertex {
    uint8_t packedPosition[3];
    uint8_t lightNormalIdx;
};

struct Frame {
    FrameVertex min;
    FrameVertex max;
    char name[16];
    vector<FrameVertex> vertices;
};

struct FrameGroup {
    FrameVertex min;
    FrameVertex max;
    vector<float> times;
    vector<Frame> frames;
};

struct ModelVertex {
    vec3 position;
};

void readFrame(FILE* file, int32_t numverts, Frame& frame) {
    readStruct(file, frame.min);
    readStruct(file, frame.max);
    readStruct(file, frame.name);
    frame.vertices.resize(numverts);
    fread(frame.vertices.data(), sizeof(FrameVertex), numverts, file);
}

void readFrameGroup(FILE* file, int32_t numverts, FrameGroup& group) {
    int32_t groupType = -1;
    fread(&groupType, sizeof(groupType), 1, file);

    int32_t frameCount = 0;
    fread(&frameCount, sizeof(frameCount), 1, file);
    if (frameCount < 1) throw std::runtime_error("invalid frame count");

    if (groupType == 0) {
        Frame frame = {};
        readFrame(file, numverts, frame);
    } else if (groupType > 0) {
        readStruct(file, group.min);
        readStruct(file, group.max);
        group.times.resize(frameCount);
        fread(group.times.data(), sizeof(float), frameCount, file);
        group.frames.resize(frameCount);
        for (int i = 0; i < frameCount; i++) {
            readFrame(file, numverts, group.frames[i]);
        }
    } else {
        throw std::runtime_error("invalid frame group type");
    }
}

void uploadMDL(
    Vulkan& vk,
    FILE* file,
    uint32_t offset,
    Palette& palette,
    VulkanMesh& mesh
) {
    MDLHeader header;
    seek(file, offset);
    readStruct(file, header);

    uint32_t group;
    readStruct(file, group);

    if (group != 0) {
        LOG(ERROR) << "group skins not supported";
        exit(-1);
    }

    uint32_t skinIdxsSize = header.skinheight * header.skinwidth;
    vector<uint8_t> skinIdxs(skinIdxsSize);
    fread(skinIdxs.data(), skinIdxsSize, 1, file);

    uint32_t skinColorsSize = skinIdxsSize * 4;
    vector<uint8_t> skinColors(skinColorsSize);

    for (uint32_t i = 0; i < skinIdxsSize; i++) {
        auto colorIdx = skinIdxs[i];
        auto paletteColor = palette.colors[colorIdx];
        skinColors[i*4] = paletteColor.r;
        skinColors[i*4+1] = paletteColor.g;
        skinColors[i*4+2] = paletteColor.b;
        skinColors[i*4+3] = 255;
    }

    VulkanSampler sampler;
    uploadTexture(
        vk.device,
        vk.memories,
        vk.queue,
        vk.queueFamily,
        vk.cmdPoolTransient,
        header.skinwidth,
        header.skinheight,
        skinColors.data(),
        skinColorsSize,
        sampler
    );

    vector<TexCoord> texCoords(header.numverts);
    fread(texCoords.data(), sizeof(TexCoord), header.numverts, file);

    vector<Triangle> triangles(header.numtris);
    fread(triangles.data(), sizeof(Triangle), header.numtris, file);

    vector<FrameGroup> groups(header.numframes);
    for (auto& group: groups) {
        readFrameGroup(file, header.numverts, group);
    }

    vector<ModelVertex> vertices;
    for (auto& group: groups) {
        auto& frame = group.frames[0];
        for (auto& packedVertex: frame.vertices) {
            auto& vertex = vertices.emplace_back();
            vertex.position.x = packedVertex.packedPosition[0]
                * header.scale.x + header.offsets.x;
            vertex.position.y = -packedVertex.packedPosition[2]
                * header.scale.z + header.offsets.z;
            vertex.position.z = packedVertex.packedPosition[1]
                * header.scale.y + header.offsets.y;
        }
    }

    vector<uint32_t> indices;
    for (auto& triangle: triangles) {
        for (int i = 0; i < 3; i ++) {
            indices.push_back(triangle.vertices[i]);
        }
    }

    uploadMesh(
        vk.device,
        vk.memories,
        vk.queueFamily,
        vertices.data(),
        vertices.size() * sizeof(ModelVertex),
        indices.data(),
        indices.size() * sizeof(uint32_t),
        mesh
    );
    mesh.vCount = vertices.size();
    mesh.idxCount = indices.size();
}

void renderModel(
    Vulkan& vk,
    PAKParser& pak,
    vector<Entity>& entities,
    vector<VkCommandBuffer>& cmds
) {
    auto entry = pak.findEntry("progs/flame2.mdl");
    auto palette = pak.loadPalette();
    VulkanMesh mesh = {};
    uploadMDL(vk, pak.file, entry.offset, *palette, mesh);
    delete palette;

    vector<vec3> origins;
    for (auto& entity: entities) {
        auto name = entity.className;
        if (strcmp(name, "light_flame_large_yellow") == 0) {
            origins.push_back(entity.origin);
        }
    }

    VulkanPipeline pipeline = {};
    initVKPipeline(vk, "alias_model", VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, pipeline);
    updateUniformBuffer(
        vk.device,
        pipeline.descriptorSet,
        0,
        vk.mvp.handle
    );

    auto framebufferCount = vk.swap.framebuffers.size();
    createCommandBuffers(
        vk.device,
        vk.cmdPool,
        framebufferCount,
        cmds
    );
    for (int idx = 0; idx < framebufferCount; idx++) {
        auto& cmd = cmds[idx];
        auto& fb = vk.swap.framebuffers[idx];

        beginFrameCommandBuffer(cmd);

        VkRenderPassBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        beginInfo.clearValueCount = 0;
        beginInfo.framebuffer = fb;
        beginInfo.renderArea.extent = vk.swap.extent;
        beginInfo.renderArea.offset = {0, 0};
        beginInfo.renderPass = vk.renderPassNoClear;

        vkCmdBeginRenderPass(cmd, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);
        VkDeviceSize offsets[] = {0};
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.handle);
        vkCmdBindDescriptorSets(
            cmd,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipeline.layout,
            0, 1,
            &pipeline.descriptorSet,
            0, nullptr
        );
        vkCmdBindVertexBuffers(cmd, 0, 1, &mesh.vBuff.handle, offsets);
        vkCmdBindIndexBuffer(cmd, mesh.iBuff.handle, 0, VK_INDEX_TYPE_UINT32);
        for (auto& origin: origins) {
            vkCmdPushConstants(
                cmd,
                pipeline.layout,
                VK_SHADER_STAGE_VERTEX_BIT,
                0,
                sizeof(origin),
                &origin
            );
            // vkCmdDraw(cmd, mesh.vCount, 1, 0, 0);
            vkCmdDrawIndexed(cmd, mesh.idxCount, 1, 0, 0, 0);
        }
        vkCmdEndRenderPass(cmd);

        endCommandBuffer(cmd);
    }
}
