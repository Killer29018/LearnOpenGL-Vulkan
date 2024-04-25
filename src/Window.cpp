#include "Window.hpp"

#include <iostream>

#include "ErrorCheck.hpp"
#include <stdexcept>

Window::Window() {}

Window::~Window()
{
    if (m_Window)
    {
        glfwDestroyWindow(m_Window);

        glfwTerminate();
    }
}

void Window::init(const std::string& name, glm::ivec2 windowSize)
{
    assert(!m_Window && "Window already Initialized");

    if (!glfwInit())
    {
        throw std::runtime_error("Failed to initialize GLFW");
    }
    m_WindowSize = windowSize;

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    m_Window = glfwCreateWindow(m_WindowSize.x, m_WindowSize.y, name.c_str(), nullptr, nullptr);

    if (!m_Window)
    {
        throw std::runtime_error("Failed to create Window");
    }

    glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    glfwSetWindowUserPointer(m_Window, this);

    glfwSetKeyCallback(m_Window, keyboardCallback);
    glfwSetCursorPosCallback(m_Window, mouseCallback);
    glfwSetWindowFocusCallback(m_Window, mouseEnterCallback);
}

bool Window::shouldClose() { return glfwWindowShouldClose(m_Window); }

void Window::getEvents() { glfwPollEvents(); }

void Window::swapBuffers() { glfwSwapBuffers(m_Window); }

VkSurfaceKHR Window::getSurface(VkInstance instance)
{
    VkSurfaceKHR surface;
    VK_CHECK(glfwCreateWindowSurface(instance, getWindow(), nullptr, &surface));
    return surface;
}

void Window::keyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    Window* w = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) glfwSetWindowShouldClose(window, true);

    KeyboardPressEvent kpEvent;
    kpEvent.keyType = key;
    kpEvent.keyAction = action;
    kpEvent.mods = mods;
    w->dispatchEvent(&kpEvent);
}

void Window::mouseCallback(GLFWwindow* window, double xPos, double yPos)
{
    static double lastX, lastY;

    Window* w = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));

    if (w->m_FirstMouse)
    {
        lastX = xPos;
        lastY = yPos;

        w->m_FirstMouse = false;
    }

    double xOffset = xPos - lastX;
    double yOffset = lastY - yPos;

    lastX = xPos;
    lastY = yPos;

    MouseMoveEvent mmEvent;
    mmEvent.xOffset = xOffset;
    mmEvent.yOffset = yOffset;
    mmEvent.xPos = xPos;
    mmEvent.yPos = yPos;
    w->dispatchEvent(&mmEvent);
}

void Window::mouseEnterCallback(GLFWwindow* window, int entered)
{
    Window* w = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
    if (entered) w->m_FirstMouse = true;
}
