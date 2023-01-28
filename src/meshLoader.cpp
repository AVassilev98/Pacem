#include "assimp/mesh.h"
#include "assimp/types.h"
#include "glm/gtx/string_cast.hpp"
#include "meshLoader.h"
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

template <typename T>[[nodiscard]] inline __attribute__((always_inline)) glm::vec3 toU32GlmVec3(const T &elem)
{
    return glm::u32vec3(elem[0], elem[1], elem[2]);
}

Mesh loadMesh(const std::string &filePath, TransferQueue &transferQueue, VmaAllocator &allocator, DeviceInfo &deviceInfo)
{
    std::cout << "Loading mesh: " << filePath << std::endl;
    const aiScene *scene = g_importer.ReadFile(filePath, aiProcess_Triangulate | aiProcess_JoinIdenticalVertices);

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

    importedMesh.meshletVertexOffsets.reserve(scene->mNumMeshes);
    importedMesh.meshletIndexOffsets.reserve(scene->mNumMeshes);
    importedMesh.meshletIndexSizes.reserve(scene->mNumMeshes);
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

        meshletIndexOffset += mesh->mNumFaces * 3;
        meshletVertexOffset += mesh->mNumVertices;

        aiColor3D matColor;
        mat->Get(AI_MATKEY_COLOR_DIFFUSE, matColor);
        glm::vec3 diffuseColor = glm::vec3(0.2588f, 0.2588f, 0.961f); // toGlmVec3(matColor);

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

void freeMesh(Mesh &mesh, VmaAllocator allocator)
{
    vmaDestroyBuffer(allocator, mesh.vkVertexBuffer.buffer, mesh.vkVertexBuffer.allocation);
    vmaDestroyBuffer(allocator, mesh.vkIndexBuffer.buffer, mesh.vkIndexBuffer.allocation);
}

void Mesh::drawMesh(VkCommandBuffer cmdBuf)
{
    VkDeviceSize offset = 0;
    vkCmdBindIndexBuffer(cmdBuf, vkIndexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdBindVertexBuffers(cmdBuf, 0, 1, &vkVertexBuffer.buffer, &offset);

    for (uint32_t i = 0; i < meshletVertexOffsets.size(); i++)
    {
        vkCmdDrawIndexed(cmdBuf, meshletIndexSizes[i], 1, meshletIndexOffsets[i], meshletVertexOffsets[i], 0);
    }
}