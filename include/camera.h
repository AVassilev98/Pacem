#pragma once

#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/trigonometric.hpp"
#include "imgui.h"
#include <chrono>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

class UserControlledCamera
{
  public:
    UserControlledCamera();
    void update();
    glm::mat4 m_view = glm::translate(glm::mat4(1.f), {0.f, -1.f, -4.f});
    glm::mat4 m_projection = glm::perspective(glm::radians(70.0f), 16.0f / 9.0f, 0.01f, 5000.0f);

  private:
    glm::vec2 m_mouseDragDelta = {0.0f, 0.0f};
    glm::vec2 m_cameraAngle = {0.0f, 0.0f};
    glm::vec3 m_translation = {0.0f, 0.0f, 0.0f};
    std::chrono::high_resolution_clock m_clock;
    std::chrono::time_point<std::chrono::high_resolution_clock> m_timeLastUpdated = {};
};