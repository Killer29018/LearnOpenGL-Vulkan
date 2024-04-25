#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "EventHandler.hpp"

class Camera : public EventObserver
{
  public:
    Camera();
    Camera(glm::vec3 position, float pitch = 0.0f, float yaw = 0.0f);
    virtual ~Camera();

    void update(double dt);

    virtual void receiveEvent(const Event* event) override;

    glm::mat4 getView();
    glm::mat4 getPerspective(glm::ivec2 windowSize);

    glm::vec3 getPosition() { return m_Position; }

  private:
    void updateVectors();

  private:
    std::unordered_map<int, bool> m_PressedKeys;

    glm::vec3 m_Position;

    glm::vec3 m_Direction;

    glm::vec3 m_Front;
    glm::vec3 m_Right;
    glm::vec3 m_Up;

    glm::vec3 m_WorldFront = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 m_WorldRight = glm::vec3(1.0f, 0.0f, 0.0f);
    glm::vec3 m_WorldUp = glm::vec3(0.0f, -1.0f, 0.0f);

    float m_MovementSpeed = 0.01f;
    float m_MovementSpeedMult = 0.1f;

    float m_vFOV = 70.0f;

    float m_Pitch;
    float m_Yaw;
};
