#pragma once
#include <string>
#include <vulkan/vulkan.h>

struct Shader
{
    enum class Stage
    {
        Vertex = 0,
        TesselationCtrl = 1,
        TesselationEval = 2,
        Geometry = 3,
        Fragment = 4,
        Compute = 5,
        AllGfx = 6,
        All = 7,
        RayGen = 8,
        RayAny = 9,
        RayClosest = 10,
        Miss = 11,
        Intersect = 12,
        Callable = 13,
        Task = 14,
        Mesh = 15,
        Num = 16,
    };

    VkShaderModule m_module;
    VkShaderStageFlagBits m_moduleFlags;

    Shader(const std::string &path, Shader::Stage stage);
    ~Shader();
};