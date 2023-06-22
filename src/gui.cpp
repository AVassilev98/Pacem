#include "GLFW/glfw3.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_vulkan.h"
#include "gui.h"
#include "imgui.h"
#include "renderer.h"
#include "types.h"
#include "vkinit.h"
#include <array>
#include <cstdint>
#include <vulkan/vulkan_core.h>

Gui &Gui::Get()
{
    static Gui gui;
    return gui;
}

void Gui::createFrameBuffers(VkRenderPass renderPass)
{
    Renderer &renderer = Renderer::Get();
    uint32_t maxFramesInFlight = renderer.getMaxNumFramesInFlight();
    const SwapchainInfo &swapchainInfo = renderer.getSwapchainInfo();

    m_swapchainImages.reserve(maxFramesInFlight);
    m_framebuffers.reserve(maxFramesInFlight);

    for (int i = 0; i < maxFramesInFlight; i++)
    {
        m_swapchainImages.emplace_back(&renderer.m_swapchainImages[i]);
        auto attachments = std::to_array<ImageRef>({
            {VK_FORMAT_B8G8R8A8_UNORM, m_swapchainImages[i]},
        });
        m_swapchainImages[i]->addImageViewFormat(VK_FORMAT_B8G8R8A8_UNORM);

        Framebuffer::State framebufferState = {
            .images = std::span(attachments),
            .renderpass = renderPass,
        };
        m_framebuffers.emplace_back(framebufferState);
    }
}

void Gui::resize(uint32_t width, uint32_t height)
{
    for (uint32_t i = 0; i < m_swapchainImages.size(); i++)
    {
        m_framebuffers[i].freeResources();
    }
    m_framebuffers.clear();
    m_swapchainImages.clear();
    createFrameBuffers(m_renderPass);
}

void Gui::setImGuiStyles()
{
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    ImGuiStyle &style = ImGui::GetStyle();

    style.Alpha = 0.800000011920929f;
    style.DisabledAlpha = 0.6000000238418579f;
    style.WindowPadding = ImVec2(6.199999809265137f, 6.199999809265137f);
    style.WindowRounding = 0.0f;
    style.WindowBorderSize = 0.0f;
    style.WindowMinSize = ImVec2(32.0f, 32.0f);
    style.WindowTitleAlign = ImVec2(0.0f, 0.5f);
    style.WindowMenuButtonPosition = ImGuiDir_None;
    style.ChildRounding = 0.0f;
    style.ChildBorderSize = 1.0f;
    style.PopupRounding = 0.0f;
    style.PopupBorderSize = 1.0f;
    style.FramePadding = ImVec2(4.0f, 4.0f);
    style.FrameRounding = 0.0f;
    style.FrameBorderSize = 0.0f;
    style.ItemSpacing = ImVec2(8.0f, 4.0f);
    style.ItemInnerSpacing = ImVec2(0.0f, 4.0f);
    style.CellPadding = ImVec2(5.0f, 5.0f);
    style.IndentSpacing = 10.0f;
    style.ColumnsMinSpacing = 5.599999904632568f;
    style.ScrollbarSize = 20.0f;
    style.ScrollbarRounding = 0.0f;
    style.GrabMinSize = 20.0f;
    style.GrabRounding = 0.0f;
    style.TabRounding = 0.0f;
    style.TabBorderSize = 1.0f;
    style.TabMinWidthForCloseButton = 0.0f;
    style.ColorButtonPosition = ImGuiDir_Right;
    style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
    style.SelectableTextAlign = ImVec2(0.0f, 0.0f);

    style.Colors[ImGuiCol_Text] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.5921568870544434f, 0.5921568870544434f, 0.5921568870544434f, 1.0f);
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.1450980454683304f, 0.1450980454683304f, 0.1490196138620377f, 1.0f);
    style.Colors[ImGuiCol_ChildBg] = ImVec4(0.1450980454683304f, 0.1450980454683304f, 0.1490196138620377f, 1.0f);
    style.Colors[ImGuiCol_PopupBg] = ImVec4(0.1450980454683304f, 0.1450980454683304f, 0.1490196138620377f, 1.0f);
    style.Colors[ImGuiCol_Border] = ImVec4(0.3058823645114899f, 0.3058823645114899f, 0.3058823645114899f, 1.0f);
    style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.3058823645114899f, 0.3058823645114899f, 0.3058823645114899f, 1.0f);
    style.Colors[ImGuiCol_FrameBg] = ImVec4(0.2000000029802322f, 0.2000000029802322f, 0.2156862765550613f, 1.0f);
    style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.1137254908680916f, 0.5921568870544434f, 0.9254902005195618f, 1.0f);
    style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.0f, 0.4666666686534882f, 0.7843137383460999f, 1.0f);
    style.Colors[ImGuiCol_TitleBg] = ImVec4(0.1450980454683304f, 0.1450980454683304f, 0.1490196138620377f, 1.0f);
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.1450980454683304f, 0.1450980454683304f, 0.1490196138620377f, 1.0f);
    style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.1450980454683304f, 0.1450980454683304f, 0.1490196138620377f, 1.0f);
    style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.2000000029802322f, 0.2000000029802322f, 0.2156862765550613f, 1.0f);
    style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.2000000029802322f, 0.2000000029802322f, 0.2156862765550613f, 1.0f);
    style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.321568638086319f, 0.321568638086319f, 0.3333333432674408f, 1.0f);
    style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.3529411852359772f, 0.3529411852359772f, 0.3725490272045135f, 1.0f);
    style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.3529411852359772f, 0.3529411852359772f, 0.3725490272045135f, 1.0f);
    style.Colors[ImGuiCol_CheckMark] = ImVec4(0.0f, 0.4666666686534882f, 0.7843137383460999f, 1.0f);
    style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.1137254908680916f, 0.5921568870544434f, 0.9254902005195618f, 1.0f);
    style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.0f, 0.4666666686534882f, 0.7843137383460999f, 1.0f);
    style.Colors[ImGuiCol_Button] = ImVec4(0.2000000029802322f, 0.2000000029802322f, 0.2156862765550613f, 1.0f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.1137254908680916f, 0.5921568870544434f, 0.9254902005195618f, 1.0f);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.1137254908680916f, 0.5921568870544434f, 0.9254902005195618f, 1.0f);
    style.Colors[ImGuiCol_Header] = ImVec4(0.2000000029802322f, 0.2000000029802322f, 0.2156862765550613f, 1.0f);
    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.1137254908680916f, 0.5921568870544434f, 0.9254902005195618f, 1.0f);
    style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.0f, 0.4666666686534882f, 0.7843137383460999f, 1.0f);
    style.Colors[ImGuiCol_Separator] = ImVec4(0.3058823645114899f, 0.3058823645114899f, 0.3058823645114899f, 1.0f);
    style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.3058823645114899f, 0.3058823645114899f, 0.3058823645114899f, 1.0f);
    style.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.3058823645114899f, 0.3058823645114899f, 0.3058823645114899f, 1.0f);
    style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.1450980454683304f, 0.1450980454683304f, 0.1490196138620377f, 1.0f);
    style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.2000000029802322f, 0.2000000029802322f, 0.2156862765550613f, 1.0f);
    style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.321568638086319f, 0.321568638086319f, 0.3333333432674408f, 1.0f);
    style.Colors[ImGuiCol_Tab] = ImVec4(0.1450980454683304f, 0.1450980454683304f, 0.1490196138620377f, 1.0f);
    style.Colors[ImGuiCol_TabHovered] = ImVec4(0.1137254908680916f, 0.5921568870544434f, 0.9254902005195618f, 1.0f);
    style.Colors[ImGuiCol_TabActive] = ImVec4(0.0f, 0.4666666686534882f, 0.7843137383460999f, 1.0f);
    style.Colors[ImGuiCol_TabUnfocused] = ImVec4(0.1450980454683304f, 0.1450980454683304f, 0.1490196138620377f, 1.0f);
    style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.0f, 0.4666666686534882f, 0.7843137383460999f, 1.0f);
    style.Colors[ImGuiCol_PlotLines] = ImVec4(0.0f, 0.4666666686534882f, 0.7843137383460999f, 1.0f);
    style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.1137254908680916f, 0.5921568870544434f, 0.9254902005195618f, 1.0f);
    style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.0f, 0.4666666686534882f, 0.7843137383460999f, 1.0f);
    style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.1137254908680916f, 0.5921568870544434f, 0.9254902005195618f, 1.0f);
    style.Colors[ImGuiCol_TableHeaderBg] = ImVec4(0.1882352977991104f, 0.1882352977991104f, 0.2000000029802322f, 1.0f);
    style.Colors[ImGuiCol_TableBorderStrong] = ImVec4(0.3098039329051971f, 0.3098039329051971f, 0.3490196168422699f, 1.0f);
    style.Colors[ImGuiCol_TableBorderLight] = ImVec4(0.2274509817361832f, 0.2274509817361832f, 0.2470588237047195f, 1.0f);
    style.Colors[ImGuiCol_TableRowBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    style.Colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.0f, 1.0f, 1.0f, 0.05999999865889549f);
    style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.0f, 0.4666666686534882f, 0.7843137383460999f, 1.0f);
    style.Colors[ImGuiCol_DragDropTarget] = ImVec4(0.1450980454683304f, 0.1450980454683304f, 0.1490196138620377f, 1.0f);
    style.Colors[ImGuiCol_NavHighlight] = ImVec4(0.1450980454683304f, 0.1450980454683304f, 0.1490196138620377f, 1.0f);
    style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.0f, 1.0f, 1.0f, 0.699999988079071f);
    style.Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.800000011920929f, 0.800000011920929f, 0.800000011920929f, 0.2000000029802322f);
    style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.1450980454683304f, 0.1450980454683304f, 0.1490196138620377f, 1.0f);
    style.Colors[ImGuiCol_DockingEmptyBg] = {0, 0, 0, 0};
}

Gui::Gui()
{
    Renderer &renderer = Renderer::Get();

    auto poolSizes = std::to_array<VkDescriptorPoolSize>({
        {VK_DESCRIPTOR_TYPE_SAMPLER, 500},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4000},
        {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 4000},
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
        {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2000},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2000},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
        {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 500},
    });

    VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    descriptorPoolCreateInfo.maxSets = 1000;
    descriptorPoolCreateInfo.poolSizeCount = poolSizes.size();
    descriptorPoolCreateInfo.pPoolSizes = poolSizes.data();
    descriptorPoolCreateInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    VK_LOG_ERR(vkCreateDescriptorPool(renderer.m_deviceInfo.device, &descriptorPoolCreateInfo, nullptr, &m_imguiDescriptorPool));

    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForVulkan(renderer.m_windowInfo.window, true);

    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = renderer.m_instance;
    init_info.PhysicalDevice = renderer.m_physDeviceInfo.device;
    init_info.Device = renderer.m_deviceInfo.device;
    init_info.Queue = renderer.m_transferQueue.graphicsQueue;
    init_info.DescriptorPool = m_imguiDescriptorPool;
    init_info.MinImageCount = renderer.getMaxNumFramesInFlight();
    init_info.ImageCount = renderer.getMaxNumFramesInFlight();
    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

    VkInit::RenderPassState::Attachment colorAttachment = {
        .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .initialLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        .format = VK_FORMAT_B8G8R8A8_UNORM,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .attachment = 0,
        .referenceLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };

    VkInit::RenderPassState renderPassState = {
        .colorAttachments = std::span(&colorAttachment, 1),
        .bindpoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
    };

    m_renderPass = VkInit::CreateVkRenderPass(renderPassState);
    createFrameBuffers(m_renderPass);

    ImGui_ImplVulkan_Init(&init_info, m_renderPass);
    renderer.graphicsImmediate(
        [&](VkCommandBuffer cmd)
        {
            ImGui_ImplVulkan_CreateFontsTexture(cmd);
        });
    ImGui_ImplVulkan_DestroyFontUploadObjects();

    setImGuiStyles();
}

Gui::~Gui()
{
    Renderer &renderer = Renderer::Get();
    for (Framebuffer framebuffer : m_framebuffers)
    {
        framebuffer.freeResources();
    }
    vkDestroyRenderPass(renderer.m_deviceInfo.device, m_renderPass, nullptr);
    vkDestroyDescriptorPool(renderer.m_deviceInfo.device, m_imguiDescriptorPool, nullptr);
    ImGui_ImplVulkan_Shutdown();
    ImGui::DestroyContext();
}

void Gui::collectImGuiFrameData()
{
    Renderer &renderer = Renderer::Get();
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    setImGuiDockspace();
    ImGui::ShowDemoWindow();
    ImGui::Render();
}

void Gui::draw(VkCommandBuffer commandBuffer, uint32_t frameIndex)
{
    Renderer &renderer = Renderer::Get();
    collectImGuiFrameData();

    VkRenderPassBeginInfo renderPassBeginInfo = {VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    VkRect2D renderArea = {};
    renderArea.extent = {m_swapchainImages[0]->m_width, m_swapchainImages[0]->m_height};
    renderPassBeginInfo.renderArea = renderArea;
    renderPassBeginInfo.renderPass = m_renderPass;

    VkExtent2D windowExtent = renderer.getWindowExtent();
    VkViewport viewport;
    viewport.height = static_cast<float>(windowExtent.height);
    viewport.width = static_cast<float>(windowExtent.width);
    viewport.x = 0;
    viewport.y = 0;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor;
    scissor.offset = {0, 0};
    scissor.extent = {(uint32_t)windowExtent.width, (uint32_t)windowExtent.height};

    renderPassBeginInfo.framebuffer = m_framebuffers[frameIndex].m_frameBuffer;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
    VkSubpassContents subpassContents = {};
    vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, subpassContents);
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
    vkCmdEndRenderPass(commandBuffer);
}

void Gui::setImGuiDockspace()
{
    static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse
                                  | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus
                                  | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBackground;
    const ImGuiViewport *viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::SetNextWindowViewport(viewport->ID);
    // Submit the DockSpace
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    ImGui::Begin("MainWindow", nullptr, window_flags);

    ImGuiIO &io = ImGui::GetIO();
    ImGuiID dockspace_id = ImGui::GetID("MainDockspace");
    ImGui::SetNextWindowBgAlpha(0.0f);
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
    ImGui::PopStyleVar(3);
    ImGui::End();
}