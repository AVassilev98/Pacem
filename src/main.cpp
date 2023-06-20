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

    Shader lineVertShader(CONCAT(SHADER_PATH, "lineDraw.vert.spv"), Shader::Stage::Vertex);
    Shader lineFragShader(CONCAT(SHADER_PATH, "lineDraw.frag.spv"), Shader::Stage::Fragment);

    Shader vertShader(CONCAT(SHADER_PATH, "default.vert.spv"), Shader::Stage::Vertex);
    Shader geoShader(CONCAT(SHADER_PATH, "default.geom.spv"), Shader::Stage::Geometry);
    Shader fragShader(CONCAT(SHADER_PATH, "default.frag.spv"), Shader::Stage::Fragment);

    auto lineShaders = std::to_array({&lineVertShader, &lineFragShader});
    auto mainShaders = std::to_array({&vertShader, &geoShader, &fragShader});

    EditorRenderPass editorRenderPass(lineShaders);
    MainRenderPass mainRenderPass(mainShaders);
    Mesh suzanneMesh(CONCAT(ASSET_PATH, "DamagedHelmet.glb"), mainRenderPass.m_pipeline);
    mainRenderPass.addMesh(&suzanneMesh);

    // renderer.addRenderPass(&editorRenderPass);
    renderer.addRenderPass(&mainRenderPass);
    renderer.addRenderPass(&gui);
    gui.declareGammaDependency(mainRenderPass.m_outputImages);

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