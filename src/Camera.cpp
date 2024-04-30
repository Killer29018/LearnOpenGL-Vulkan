#include "Camera.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

#include <format>
#include <iostream>

Camera::Camera() : m_Position{ 0.0f }, m_Direction{ 0.0f }, m_Pitch{ 0.0f }, m_Yaw{ 0.0f }
{
    updateVectors();
}

Camera::Camera(glm::vec3 position, float pitch, float yaw)
    : m_Position{ position }, m_Direction{ 0.0f }, m_Pitch{ pitch }, m_Yaw{ yaw }
{
    updateVectors();
}

Camera::~Camera() {}

void Camera::update(double dt)
{
    m_Direction = glm::vec3{ 0.0f };
    float speed = m_MovementSpeed;

    if (m_PressedKeys[GLFW_KEY_W]) m_Direction += m_Front;
    if (m_PressedKeys[GLFW_KEY_A]) m_Direction += m_Right;
    if (m_PressedKeys[GLFW_KEY_S]) m_Direction += -m_Front;
    if (m_PressedKeys[GLFW_KEY_D]) m_Direction += -m_Right;
    if (m_PressedKeys[GLFW_KEY_SPACE]) m_Direction += m_WorldUp;
    if (m_PressedKeys[GLFW_KEY_LEFT_CONTROL]) m_Direction += -m_WorldUp;
    if (m_PressedKeys[GLFW_KEY_LEFT_SHIFT]) speed *= m_MovementSpeedMult;

    if (glm::length(m_Direction) != 0) m_Direction = glm::normalize(m_Direction);

    m_Position += m_Direction * speed * (float)dt;

    // std::cout << std::format("{} | {}\n", m_Pitch, m_Yaw);
}

void Camera::receiveEvent(const Event* event)
{
    switch (event->getType())
    {
    case EventType::KEYBOARD_PRESS:
        {
            const KeyboardPressEvent* kpEvent = reinterpret_cast<const KeyboardPressEvent*>(event);

            if (kpEvent->keyAction == GLFW_PRESS) m_PressedKeys[kpEvent->keyType] = true;
            if (kpEvent->keyAction == GLFW_RELEASE) m_PressedKeys[kpEvent->keyType] = false;

            break;
        }
    case EventType::MOUSE_MOVE:
        {
            const MouseMoveEvent* mmEvent = reinterpret_cast<const MouseMoveEvent*>(event);

            const float mouseSensitivity = 0.1f;
            m_Yaw += mmEvent->xOffset * mouseSensitivity;
            m_Pitch -= mmEvent->yOffset * mouseSensitivity;

            m_Pitch = std::clamp(m_Pitch, -89.0f, 89.0f);

            break;
        }
    default:
        break;
    }

    updateVectors();
}

glm::mat4 Camera::getView() { return glm::lookAt(m_Position, m_Position + m_Front, m_WorldUp); }

glm::mat4 Camera::getPerspective(glm::ivec2 windowSize)
{
    const float near = 0.01f;
    const float far = 1000.0f;
    glm::mat4 proj = glm::perspective(glm::radians(m_vFOV),
                                      (float)windowSize.x / (float)windowSize.y, far, near);
    proj[1][1] *= -1;
    return proj;
}

void Camera::updateVectors()
{
    glm::vec3 direction;
    float yaw = -(m_Yaw + 90.0f);
    float pitch = m_Pitch;
    direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    direction.y = sin(glm::radians(pitch));
    direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));

    m_Front = glm::normalize(direction);
    m_Right = -glm::cross(m_Front, m_WorldUp);
    m_Up = glm::cross(m_Right, m_Front);
}
