#pragma once
#include "Pipeline.h"
#include "ResourcePool.h"
#include "Types.h"
#include <vector>
#include <vulkan/vulkan.h>

struct Mesh
{
    Mesh(const std::string &path, const GraphicsPipeline &pipeline);
    ~Mesh();

    VkSampler sampler;

    // Total Mesh Buffer
    std::vector<Vertex> vertices;
    std::vector<glm::u32vec3> faces;
    Handle<Buffer> vkVertexBuffer;
    Handle<Buffer> vkIndexBuffer;

    // Per Submesh
    std::vector<VkDeviceSize> meshletVertexOffsets;
    std::vector<VkDeviceSize> meshletIndexOffsets;
    std::vector<VkDeviceSize> meshletIndexSizes;
    std::vector<VkDeviceSize> matIndex;

    // All Textures
    std::unordered_map<std::string, Handle<Image>> textures;

    // Per Material
    std::vector<VkDescriptorSet> matDescriptorSets;

    const GraphicsPipeline &m_parentPipeline;

    void drawMesh(VkCommandBuffer cmdBuf, VkPipelineLayout pipelineLayout);
};
