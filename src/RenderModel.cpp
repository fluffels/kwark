#pragma warning(disable: 4267)

#include "BSPParser.h"
#include "FileSystem.h"
#include "Model.h"
#include "Palette.h"
#include "PAKParser.h"
#include "Vulkan.h"
#include "VulkanImage.h"
#include "VulkanPipeline.h"

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
    long facesfront;  // boolean
    long vertices[3]; // vertex indices [0, numverts]
};

void uploadMDL(Vulkan& vk, FILE* file, uint32_t offset, Palette& palette) {
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
}

void renderModel(
    Vulkan& vk,
    PAKParser& pak,
    vector<Entity>& entities,
    vector<VkCommandBuffer>& cmds
) {
    auto entry = pak.findEntry("progs/flame2.mdl");
    auto palette = pak.loadPalette();
    uploadMDL(vk, pak.file, entry.offset, *palette);
    delete palette;

    for (auto& entity: entities) {
        auto name = entity.className;

        if (strcmp(name, "light_flame_large_yellow") == 0) {

        }

        auto origin = entity.origin;
    }

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
        vkCmdEndRenderPass(cmd);

        endCommandBuffer(cmd);
    }
}