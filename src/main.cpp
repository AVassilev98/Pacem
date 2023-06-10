#include "array"
#include "common.h"
#include "glm/fwd.hpp"
#include "iostream"
#include "vector"
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vulkan/vulkan_core.h>

#define VMA_IMPLEMENTATION
#include "vma.h"

#include "common.h"
#include "gui.h"
#include "pipeline.h"
#include "renderer.h"
#include "renderpass.h"
#include "shader.h"
#include "types.h"
#include "vkinit.h"

int main()
{
    if (glfwInit() != GLFW_TRUE)
    {
        std::cerr << "Could not init glfw" << std::endl;
        return -1;
    }

    Renderer &renderer = Renderer::Get();
    Gui &gui = Gui::Get();

    std::string vertShaderPath = CONCAT(SHADER_PATH, "default.vert.spv");
    std::string geoShaderPath = CONCAT(SHADER_PATH, "default.geom.spv");
    std::string fragShaderPath = CONCAT(SHADER_PATH, "default.frag.spv");

    std::string suzannePath = CONCAT(ASSET_PATH, "DamagedHelmet.glb");

    Shader vertShader(vertShaderPath, Shader::Stage::Vertex);
    Shader geoShader(geoShaderPath, Shader::Stage::Geometry);
    Shader fragShader(fragShaderPath, Shader::Stage::Fragment);

    auto shadersp = std::to_array({&vertShader, &geoShader, &fragShader});

    MainRenderPass mainRenderPass(shadersp);
    Mesh suzanneMesh(suzannePath, mainRenderPass.m_pipeline);
    mainRenderPass.addMesh(&suzanneMesh);

    renderer.addRenderPass(&mainRenderPass);
    renderer.addRenderPass(&gui);

    double lastTime = glfwGetTime();
    int nbFrames = 0;

    while (!renderer.exitSignal())
    {
        VkResult drawStatus = renderer.draw();

        double currentTime = glfwGetTime();
        nbFrames++;
        if (currentTime - lastTime >= 1.0)
        {
            printf("%f ms/frame\n", 1000.0 / double(nbFrames));
            nbFrames = 0;
            lastTime += 1.0;
        }
        glfwPollEvents();
    }
    renderer.wait();

    return 0;
}