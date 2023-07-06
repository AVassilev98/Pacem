#include "Common.h"
#include "Renderer.h"
#include "Shader.h"
#include <fstream>
#include <stdexcept>
#include <vulkan/vulkan_core.h>

namespace
{
    VkShaderStageFlagBits StageToVkFlag(Shader::Stage stage)
    {
        // clang-format off
        switch (stage)
        {
            case Shader::Stage::Vertex:             return VK_SHADER_STAGE_VERTEX_BIT;
            case Shader::Stage::TesselationCtrl:    return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
            case Shader::Stage::TesselationEval:    return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
            case Shader::Stage::Geometry:           return VK_SHADER_STAGE_GEOMETRY_BIT;
            case Shader::Stage::Fragment:           return VK_SHADER_STAGE_FRAGMENT_BIT;
            case Shader::Stage::Compute:            return VK_SHADER_STAGE_COMPUTE_BIT;
            case Shader::Stage::AllGfx:             return VK_SHADER_STAGE_ALL_GRAPHICS;
            case Shader::Stage::All:                return VK_SHADER_STAGE_ALL;
            case Shader::Stage::RayGen:             return VK_SHADER_STAGE_RAYGEN_BIT_KHR;
            case Shader::Stage::RayAny:             return VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
            case Shader::Stage::RayClosest:         return VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
            case Shader::Stage::Miss:               return VK_SHADER_STAGE_MISS_BIT_KHR;
            case Shader::Stage::Intersect:          return VK_SHADER_STAGE_INTERSECTION_BIT_KHR;
            case Shader::Stage::Callable:           return VK_SHADER_STAGE_CALLABLE_BIT_KHR;
            case Shader::Stage::Task:               return VK_SHADER_STAGE_TASK_BIT_EXT;
            case Shader::Stage::Mesh:               return VK_SHADER_STAGE_MESH_BIT_EXT;
            default:
            {
                throw std::invalid_argument("Invalid shader stage supplied.");
            }
        }
        // clang-format on
    }
}; // namespace

Shader::Shader(const std::string &path, Shader::Stage stage)
    : m_moduleFlags(StageToVkFlag(stage))
{
    Renderer &renderer = Renderer::Get();
    std::ifstream file(path, std::ios::ate | std::ios::binary);

    if (!file.is_open())
    {
        throw std::invalid_argument("Could not open file " + path);
    }

    size_t fileSize = (size_t)file.tellg();
    std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));
    file.seekg(0);
    file.read((char *)buffer.data(), fileSize);
    file.close();

    VkShaderModuleCreateInfo shaderModuleCreateInfo = {VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    shaderModuleCreateInfo.codeSize = fileSize;
    shaderModuleCreateInfo.pCode = buffer.data();

    VK_LOG_ERR(vkCreateShaderModule(renderer.getDevice(), &shaderModuleCreateInfo, nullptr, &m_module));
}

Shader::~Shader()
{
    Renderer &renderer = Renderer::Get();
    vkDestroyShaderModule(renderer.getDevice(), m_module, nullptr);
}