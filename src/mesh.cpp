#include <array>
#include <iostream>
#include <vulkan/vulkan_core.h>

#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "assimp/scene.h"
#include "common.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "gpuresource.h"
#include "mesh.h"
#include "renderer.h"
#include "vkinit.h"

namespace
{
    static Assimp::Importer g_importer;

    template <typename T> [[nodiscard]] inline glm::vec3 toGlmVec3(const T &elem)
    {
        return glm::vec3(elem[0], elem[1], elem[2]);
    }

    template <typename T> [[nodiscard]] inline glm::vec2 toGlmVec2(const T &elem)
    {
        return glm::vec2(elem[0], -elem[1]);
    }

    template <typename T> [[nodiscard]] inline glm::vec3 toU32GlmVec3(const T &elem)
    {
        return glm::u32vec3(elem[0], elem[1], elem[2]);
    }
}; // namespace

Mesh::Mesh(const std::string &path, const Pipeline &pipeline)
    : m_parentPipeline(pipeline)
{
    Renderer &renderer = Renderer::Get();
    std::cout << "Loading mesh: " << path << std::endl;
    const aiScene *scene = g_importer.ReadFile(path, aiProcessPreset_TargetRealtime_Quality | aiProcess_FindInstances
                                                         | aiProcess_ValidateDataStructure | aiProcess_OptimizeMeshes | aiProcess_Debone);

    if (!scene)
    {
        std::cout << "Import Failed: " << g_importer.GetErrorString() << std::endl;
    }

    size_t totalVertexCount = 0;
    size_t totalFaceCount = 0;
    size_t meshletVertexOffset = 0;
    size_t meshletIndexOffset = 0;

    for (size_t i = 0; i < scene->mNumMeshes; i++)
    {
        aiMesh *mesh = scene->mMeshes[i];
        totalVertexCount += mesh->mNumVertices;
        totalFaceCount += mesh->mNumFaces;
    }

    for (size_t i = 0; i < scene->mNumTextures; i++)
    {
        aiTexture *texture = scene->mTextures[i];

        int width = texture->mWidth;
        int height = texture->mHeight;
        int numComponents = 0;

        std::string path = scene->mTextures[i]->mFilename.length ? scene->mTextures[i]->mFilename.C_Str() : "";
        void *texelBuffer = scene->mTextures[i]->pcData;

        // if image is compressed the height will be 0 and width will be the number of bytes
        if (!height)
        {
            texelBuffer = (aiTexel *)stbi_load_from_memory((stbi_uc *)scene->mTextures[i]->pcData, texture->mWidth, &width, &height,
                                                           &numComponents, STBI_rgb_alpha);
            path = std::string("*" + std::to_string(i));
        }
        printf("%s: [%d, %d, %d]\n", texture->mFilename.C_Str(), width, height, STBI_rgb_alpha);

        uint32_t bufferSize = width * height * STBI_rgb_alpha;
        textures.insert({path, renderer.uploadTextureToGpu(std::span((uint8_t *)texelBuffer, bufferSize), width, height)});
    }

    // default sampler
    sampler = VkInit::CreateVkSampler({});

    for (size_t i = 0; i < scene->mNumMaterials; i++)
    {
        aiString diffusePath;
        aiString aoPath;
        aiString emissivePath;
        aiString normalPath;

        aiMaterial *mat = scene->mMaterials[i];
        mat->GetTexture(aiTextureType_DIFFUSE, 0, &diffusePath);
        mat->GetTexture(aiTextureType_LIGHTMAP, 0, &aoPath);
        mat->GetTexture(aiTextureType_EMISSIVE, 0, &emissivePath);
        mat->GetTexture(aiTextureType_NORMALS, 0, &normalPath);

        VkDescriptorImageInfo descriptorImageInfo = {};
        descriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        descriptorImageInfo.sampler = sampler;
        matDescriptorSets.push_back(renderer.allocateDescriptorSet(m_parentPipeline.m_descriptorSetLayouts[DSL_FREQ_PER_MAT]));

        if (textures.count(std::string(diffusePath.C_Str())))
        {
            printf("Found diffuse texture: %s in texture map!\n", diffusePath.C_Str());
            Image &diffuseTexture = textures.at(std::string(diffusePath.C_Str()));
            descriptorImageInfo.imageView = diffuseTexture.m_imageView;
            renderer.updateDescriptor(matDescriptorSets.back(), descriptorImageInfo, 0);
        }
        if (textures.count(std::string(aoPath.C_Str())))
        {
            printf("Found ambient occlusion texture: %s in texture map!\n", aoPath.C_Str());
            Image &aoTexture = textures.at(std::string(aoPath.C_Str()));
            descriptorImageInfo.imageView = aoTexture.m_imageView;
            renderer.updateDescriptor(matDescriptorSets.back(), descriptorImageInfo, 1);
        }
        if (textures.count(std::string(emissivePath.C_Str())))
        {
            printf("Found emissive texture: %s in texture map!\n", emissivePath.C_Str());
            Image &emissiveTexture = textures.at(std::string(emissivePath.C_Str()));
            descriptorImageInfo.imageView = emissiveTexture.m_imageView;
            renderer.updateDescriptor(matDescriptorSets.back(), descriptorImageInfo, 2);
        }
        if (textures.count(std::string(normalPath.C_Str())))
        {
            printf("Found normal map: %s in texture map!\n", normalPath.C_Str());
            Image &normalTexture = textures.at(std::string(normalPath.C_Str()));
            descriptorImageInfo.imageView = normalTexture.m_imageView;
            renderer.updateDescriptor(matDescriptorSets.back(), descriptorImageInfo, 3);
        }
    }

    meshletVertexOffsets.reserve(scene->mNumMeshes);
    meshletIndexOffsets.reserve(scene->mNumMeshes);
    meshletIndexSizes.reserve(scene->mNumMeshes);
    matIndex.reserve(scene->mNumMeshes);
    vertices.reserve(totalVertexCount);
    faces.reserve(totalFaceCount);

    for (size_t i = 0; i < scene->mNumMeshes; i++)
    {
        const aiMesh *mesh = scene->mMeshes[i];
        const uint32_t matIdx = mesh->mMaterialIndex;
        const aiMaterial *mat = scene->mMaterials[matIdx];

        meshletVertexOffsets.push_back(meshletVertexOffset);
        meshletIndexOffsets.push_back(meshletIndexOffset);
        meshletIndexSizes.push_back(mesh->mNumFaces * 3);
        matIndex.push_back(mesh->mMaterialIndex);

        meshletIndexOffset += mesh->mNumFaces * 3;
        meshletVertexOffset += mesh->mNumVertices;

        aiColor3D matColor;
        mat->Get(AI_MATKEY_COLOR_DIFFUSE, matColor);
        glm::vec3 diffuseColor = toGlmVec3(matColor);

        for (size_t j = 0; j < mesh->mNumVertices; j++)
        {
            vertices.emplace_back();
            Vertex &backVertex = vertices.back();
            backVertex.color = diffuseColor;
            if (mesh->HasPositions())
            {
                const aiVector3D &vertex = mesh->mVertices[j];
                backVertex.position = toGlmVec3(vertex);
            }

            if (mesh->HasNormals())
            {
                const aiVector3D &normal = mesh->mNormals[j];
                backVertex.normal = toGlmVec3(normal);
            }

            if (mesh->HasTextureCoords(0))
            {
                backVertex.textureCoordinate = toGlmVec2(mesh->mTextureCoords[0][j]);
            }
        }

        for (size_t j = 0; j < mesh->mNumFaces; j++)
        {
            faces.emplace_back(toU32GlmVec3(mesh->mFaces[j].mIndices));
        }
    }

    vkVertexBuffer = renderer.uploadCpuBufferToGpu(std::span(vertices), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    vkIndexBuffer = renderer.uploadCpuBufferToGpu(std::span(faces), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
}

void Mesh::drawMesh(VkCommandBuffer cmdBuf, VkPipelineLayout pipelineLayout)
{
    VkDeviceSize offset = 0;
    vkCmdBindIndexBuffer(cmdBuf, vkIndexBuffer.m_buffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdBindVertexBuffers(cmdBuf, 0, 1, &vkVertexBuffer.m_buffer, &offset);

    for (uint32_t i = 0; i < meshletVertexOffsets.size(); i++)
    {
        VkDescriptorSet descriptorSet = matDescriptorSets[matIndex[i]];

        vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, DSL_FREQ_PER_MAT, 1, &descriptorSet, 0, nullptr);
        vkCmdDrawIndexed(cmdBuf, meshletIndexSizes[i], 1, meshletIndexOffsets[i], meshletVertexOffsets[i], 0);
    }
}

Mesh::~Mesh()
{
    Renderer &renderer = Renderer::Get();

    vmaDestroyBuffer(renderer.m_vmaAllocator, vkVertexBuffer.m_buffer, vkVertexBuffer.m_allocation);
    vmaDestroyBuffer(renderer.m_vmaAllocator, vkIndexBuffer.m_buffer, vkIndexBuffer.m_allocation);
    vkFreeDescriptorSets(renderer.m_deviceInfo.device, renderer.m_descriptorPools.back(), matDescriptorSets.size(),
                         matDescriptorSets.data());
    vkDestroySampler(renderer.m_deviceInfo.device, sampler, nullptr);
    for (auto &kv : textures)
    {
        vkDestroyImageView(renderer.m_deviceInfo.device, kv.second.m_imageView, nullptr);
        vmaDestroyImage(renderer.m_vmaAllocator, kv.second.m_image, kv.second.m_allocation);
    }
}