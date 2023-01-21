#include "assimp/mesh.h"
#include "assimp/types.h"
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

// static AllocatedBuffer uploadIndexBuffer(const VmaAllocator &allocator, const VkDevice device, const std::vector<glm::vec3> &vertices)
// {
//     VkBufferCreateInfo bufferCreateInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
//     bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
// }

// static AllocatedBuffer uploadVertexBuffer(const VmaAllocator &allocator, const VkDevice device, const std::vector<Vertex> &vertices)
// {
// }

Mesh loadMesh(const std::string &filePath, TransferQueue &transferQueue, VmaAllocator &allocator, DeviceInfo &deviceInfo)
{
    std::cout << "Loading mesh: " << filePath << std::endl;
    const aiScene *scene =
        g_importer.ReadFile(filePath, aiProcess_CalcTangentSpace | aiProcess_Triangulate | aiProcess_JoinIdenticalVertices | aiProcess_SortByPType);

    Mesh importedMesh;

    size_t totalVertexCount = 0;
    size_t totalFaceCount = 0;
    for (size_t i = 0; i < scene->mNumMeshes; i++)
    {
        aiMesh *mesh = scene->mMeshes[i];
        totalVertexCount += mesh->mNumVertices;
        totalFaceCount += mesh->mNumFaces;
    }

    importedMesh.vertices.reserve(totalVertexCount);
    importedMesh.faces.reserve(totalFaceCount);

    for (size_t i = 0; i < scene->mNumMeshes; i++)
    {
        const aiMesh *mesh = scene->mMeshes[i];
        const uint32_t matIdx = mesh->mMaterialIndex;
        const aiMaterial *mat = scene->mMaterials[matIdx];

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
        }

        for (size_t j = 0; j < mesh->mNumFaces; j++)
        {
            importedMesh.faces.emplace_back(toGlmVec3(mesh->mFaces[j].mIndices));
        }
    }

    uploadBuffer(allocator, importedMesh.vertices, deviceInfo, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, transferQueue);
    return std::move(importedMesh);
}