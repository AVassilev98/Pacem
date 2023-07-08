#include "Common.h"
#include "array"
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

#include "Camera.h"
#include "Common.h"
#include "Gui.h"
#include "Pipeline.h"
#include "RenderPass.h"
#include "Renderer.h"
#include "Shader.h"
#include "Singleton.h"
#include "Types.h"
#include "VkInit.h"

int main()
{
    if (glfwInit() != GLFW_TRUE)
    {
        std::cerr << "Could not init glfw" << std::endl;
        return -1;
    }

    Renderer &renderer = Renderer::Get();
    renderer.createSwapchainImages();

    Shader lineVertShader(CONCAT(SHADER_PATH, "editorGrid.vert.spv"), Shader::Stage::Vertex);
    Shader lineFragShader(CONCAT(SHADER_PATH, "editorGrid.frag.spv"), Shader::Stage::Fragment);

    Shader vertShader(CONCAT(SHADER_PATH, "mainPass.vert.spv"), Shader::Stage::Vertex);
    Shader fragShader(CONCAT(SHADER_PATH, "mainPass.frag.spv"), Shader::Stage::Fragment);

    Shader lightCullShader(CONCAT(SHADER_PATH, "lightCull.comp.spv"), Shader::Stage::Compute);
    Shader lightShadeShader(CONCAT(SHADER_PATH, "lightShade.comp.spv"), Shader::Stage::Compute);

    auto lineShaders = std::to_array({&lineVertShader, &lineFragShader});
    auto mainShaders = std::to_array({&vertShader, &fragShader});

    UserControlledCamera mainCamera;

    EditorRenderPass editorRenderPass(lineShaders, mainCamera);
    DeferredRenderPass mainRenderPass(mainShaders, mainCamera);
    ShadingRenderPass shadingRenderPass(lightCullShader, lightShadeShader, mainCamera);
    Gui &gui = Gui::Get();

    Mesh suzanneMesh(CONCAT(ASSET_PATH, "DamagedHelmet.glb"), mainRenderPass.m_pipeline);

    renderer.addRenderPass(&editorRenderPass);

    renderer.addRenderPass(&mainRenderPass);
    mainRenderPass.declareImageDependency(editorRenderPass.m_depthImages);

    renderer.addRenderPass(&shadingRenderPass);
    shadingRenderPass.declareGBufferDependency(mainRenderPass.m_gBuffer);

    renderer.addRenderPass(&gui);
    mainRenderPass.addMesh(&suzanneMesh);

    double lastTime = glfwGetTime();
    int nbFrames = 0;

    while (!renderer.exitSignal())
    {
        mainCamera.update();
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