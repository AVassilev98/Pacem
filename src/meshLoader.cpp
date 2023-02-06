#include "assimp/material.h"
#include "assimp/mesh.h"
#include "assimp/types.h"
#include "glm/fwd.hpp"
#include "glm/gtx/string_cast.hpp"
#include "meshLoader.h"
#include <string>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <iostream>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

static Assimp::Importer g_importer;

template <typename T>[[nodiscard]] inline __attribute__((always_inline)) glm::vec3 toGlmVec3(const T &elem)
{
    return glm::vec3(elem[0], elem[1], elem[2]);
}

template <typename T>[[nodiscard]] inline __attribute__((always_inline)) glm::vec2 toGlmVec2(const T &elem)
{
    return glm::vec2(elem[0], -elem[1]);
}

template <typename T>[[nodiscard]] inline __attribute__((always_inline)) glm::vec3 toU32GlmVec3(const T &elem)
{
    return glm::u32vec3(elem[0], elem[1], elem[2]);
}

Mesh loadMesh(const std::string &filePath, TransferQueue &transferQueue, VmaAllocator &allocator, DeviceInfo &deviceInfo, VkDescriptorPool descriptorPool,
              VkDescriptorSetLayout descriptorSetLayout)
{
    std::cout << "Loading mesh: " << filePath << std::endl;
    const aiScene *scene = g_importer.ReadFile(filePath, aiProcessPreset_TargetRealtime_Quality | aiProcess_FindInstances | aiProcess_ValidateDataStructure |
                                                             aiProcess_OptimizeMeshes | aiProcess_Debone);

    if (!scene)
    {
        std::cout << "Import Failed: " << g_importer.GetErrorString() << std::endl;
    }

    Mesh importedMesh;
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
            texelBuffer =
                (aiTexel *)stbi_load_from_memory((stbi_uc *)scene->mTextures[i]->pcData, texture->mWidth, &width, &height, &numComponents, STBI_rgb_alpha);
            path = std::string("*" + std::to_string(i));
        }
        std::cout << texture->mFilename.C_Str() << ": [" << width << ", " << height << "]" << std::endl;
        printf("number of channels: %d\n", numComponents);

        Texture tex = uploadTexture(width, height, STBI_rgb_alpha, texelBuffer, transferQueue, allocator, deviceInfo, VK_FORMAT_R8G8B8A8_UNORM);
        importedMesh.textures.insert({path, tex});
    }

    importedMesh.sampler = createSampler(deviceInfo.device);

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
        descriptorImageInfo.sampler = importedMesh.sampler;
        importedMesh.matDescriptorSets.push_back(createDescriptorSet(deviceInfo.device, descriptorPool, descriptorSetLayout));

        if (importedMesh.textures.count(std::string(diffusePath.C_Str())))
        {
            printf("Found diffuse texture: %s in texture map!\n", diffusePath.C_Str());
            Texture &diffuseTexture = importedMesh.textures.at(std::string(diffusePath.C_Str()));
            descriptorImageInfo.imageView = diffuseTexture.imageView;
            updatePerMeshDescriptor(deviceInfo.device, importedMesh.matDescriptorSets.back(), descriptorImageInfo, 0);
        }
        if (importedMesh.textures.count(std::string(aoPath.C_Str())))
        {
            printf("Found ambient occlusion texture: %s in texture map!\n", aoPath.C_Str());
            Texture &aoTexture = importedMesh.textures.at(std::string(aoPath.C_Str()));
            descriptorImageInfo.imageView = aoTexture.imageView;
            updatePerMeshDescriptor(deviceInfo.device, importedMesh.matDescriptorSets.back(), descriptorImageInfo, 1);
        }
        if (importedMesh.textures.count(std::string(emissivePath.C_Str())))
        {
            printf("Found emissive texture: %s in texture map!\n", emissivePath.C_Str());
            Texture &emissiveTexture = importedMesh.textures.at(std::string(emissivePath.C_Str()));
            descriptorImageInfo.imageView = emissiveTexture.imageView;
            updatePerMeshDescriptor(deviceInfo.device, importedMesh.matDescriptorSets.back(), descriptorImageInfo, 2);
        }
        if (importedMesh.textures.count(std::string(normalPath.C_Str())))
        {
            printf("Found normal map: %s in texture map!\n", normalPath.C_Str());
            Texture &normalTexture = importedMesh.textures.at(std::string(normalPath.C_Str()));
            descriptorImageInfo.imageView = normalTexture.imageView;
            updatePerMeshDescriptor(deviceInfo.device, importedMesh.matDescriptorSets.back(), descriptorImageInfo, 3);
        }
    }

    importedMesh.meshletVertexOffsets.reserve(scene->mNumMeshes);
    importedMesh.meshletIndexOffsets.reserve(scene->mNumMeshes);
    importedMesh.meshletIndexSizes.reserve(scene->mNumMeshes);
    importedMesh.matIndex.reserve(scene->mNumMeshes);
    importedMesh.vertices.reserve(totalVertexCount);
    importedMesh.faces.reserve(totalFaceCount);

    for (size_t i = 0; i < scene->mNumMeshes; i++)
    {
        const aiMesh *mesh = scene->mMeshes[i];
        const uint32_t matIdx = mesh->mMaterialIndex;
        const aiMaterial *mat = scene->mMaterials[matIdx];

        importedMesh.meshletVertexOffsets.push_back(meshletVertexOffset);
        importedMesh.meshletIndexOffsets.push_back(meshletIndexOffset);
        importedMesh.meshletIndexSizes.push_back(mesh->mNumFaces * 3);
        importedMesh.matIndex.push_back(mesh->mMaterialIndex);

        meshletIndexOffset += mesh->mNumFaces * 3;
        meshletVertexOffset += mesh->mNumVertices;

        aiColor3D matColor;
        mat->Get(AI_MATKEY_COLOR_DIFFUSE, matColor);
        glm::vec3 diffuseColor = toGlmVec3(matColor);

        for (size_t j = 0; j < mesh->mNumVertices; j++)
        {
            importedMesh.vertices.emplace_back();
            Vertex &backVertex = importedMesh.vertices.back();
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
            importedMesh.faces.emplace_back(toU32GlmVec3(mesh->mFaces[j].mIndices));
        }
    }

    importedMesh.vkVertexBuffer = uploadBuffer(allocator, importedMesh.vertices, deviceInfo, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, transferQueue);
    importedMesh.vkIndexBuffer = uploadBuffer(allocator, importedMesh.faces, deviceInfo, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, transferQueue);
    return std::move(importedMesh);
}

void freeMesh(Mesh &mesh, VkDevice device, VmaAllocator allocator, VkDescriptorPool descriptorPool)
{
    vmaDestroyBuffer(allocator, mesh.vkVertexBuffer.buffer, mesh.vkVertexBuffer.allocation);
    vmaDestroyBuffer(allocator, mesh.vkIndexBuffer.buffer, mesh.vkIndexBuffer.allocation);
    vkFreeDescriptorSets(device, descriptorPool, mesh.matDescriptorSets.size(), mesh.matDescriptorSets.data());
    vkDestroySampler(device, mesh.sampler, nullptr);
    for (auto &kv : mesh.textures)
    {
        vkDestroyImageView(device, kv.second.imageView, nullptr);
        vmaDestroyImage(allocator, kv.second.image, kv.second.allocation);
    }
}

void Mesh::drawMesh(VkCommandBuffer cmdBuf, VkPipelineLayout pipelineLayout)
{
    VkDeviceSize offset = 0;
    vkCmdBindIndexBuffer(cmdBuf, vkIndexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdBindVertexBuffers(cmdBuf, 0, 1, &vkVertexBuffer.buffer, &offset);

    for (uint32_t i = 0; i < meshletVertexOffsets.size(); i++)
    {
        VkDescriptorSet descriptorSet = matDescriptorSets[matIndex[i]];

        vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
        vkCmdDrawIndexed(cmdBuf, meshletIndexSizes[i], 1, meshletIndexOffsets[i], meshletVertexOffsets[i], 0);
    }
}