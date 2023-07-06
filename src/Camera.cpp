#include "Camera.h"
#include "glm/common.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/quaternion_trigonometric.hpp"
#include "glm/trigonometric.hpp"
#include "imgui.h"
#include <chrono>
#include <glm/gtx/quaternion.hpp>
#include <iostream>

UserControlledCamera::UserControlledCamera()
{
    m_projection[1][1] *= -1;
}

void UserControlledCamera::update()
{
    constexpr uint32_t baseCameraSpeed = 50.0f; // 20m/s
    auto currentTime = m_clock.now();
    auto timeDelta = (currentTime - m_timeLastUpdated);
    uint64_t usecDelta = std::chrono::duration_cast<std::chrono::microseconds>(timeDelta).count();
    float secDelta = usecDelta / 10E6f;
    m_timeLastUpdated = currentTime;

    float cameraSpeed = baseCameraSpeed * secDelta;
    cameraSpeed = ImGui::IsKeyDown(ImGuiKey_LeftCtrl) ? cameraSpeed * 2 : cameraSpeed;

    float forwardSpeed = ImGui::IsKeyDown(ImGuiKey_W) ? cameraSpeed : 0;
    float backwardSpeed = ImGui::IsKeyDown(ImGuiKey_S) ? -cameraSpeed : 0;
    float leftSpeed = ImGui::IsKeyDown(ImGuiKey_A) ? cameraSpeed : 0;
    float rightSpeed = ImGui::IsKeyDown(ImGuiKey_D) ? -cameraSpeed : 0;
    float upSpeed = ImGui::IsKeyDown(ImGuiKey_Space) ? -cameraSpeed : 0;
    float downSpeed = ImGui::IsKeyDown(ImGuiKey_LeftShift) ? cameraSpeed : 0;

    if (ImGui::IsKeyDown(ImGuiKey_MouseLeft))
    {
        ImVec2 imMouseDragDelta = ImGui::GetMouseDragDelta();
        glm::vec2 mouseDragDeltaCurFrame = glm::vec2(imMouseDragDelta.x, imMouseDragDelta.y);
        glm::vec2 mouseDeltaFromLastFrame = mouseDragDeltaCurFrame - m_mouseDragDelta;
        m_mouseDragDelta = mouseDragDeltaCurFrame;
        m_cameraAngle += mouseDeltaFromLastFrame;

        m_cameraAngle.y = glm::clamp(m_cameraAngle.y, -90.0f, 90.0f);
    }
    else
    {
        m_mouseDragDelta = {0, 0};
    }

    glm::vec3 translationVec = glm::vec3(leftSpeed + rightSpeed, downSpeed + upSpeed, backwardSpeed + forwardSpeed);
    m_translation += translationVec;

    m_view = glm::mat4(1.0f);
    m_view = glm::translate(m_view, m_translation);
    m_view = glm::rotate(m_view, glm::radians(m_cameraAngle.y), {1, 0, 0});
    m_view = glm::rotate(m_view, glm::radians(m_cameraAngle.x), {0, 1, 0});
}