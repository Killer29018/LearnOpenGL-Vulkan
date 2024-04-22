#include "Window.hpp"

#include <iostream>

#include "ErrorCheck.hpp"

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
        std::runtime_error("Failed to initialize GLFW");
    }
    m_WindowSize = windowSize;

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    m_Window = glfwCreateWindow(m_WindowSize.x, m_WindowSize.y, name.c_str(), nullptr, nullptr);

    if (!m_Window)
    {
        std::runtime_error("Failed to create Window");
    }

    glfwSetWindowUserPointer(m_Window, this);

    glfwSetKeyCallback(m_Window, keyboardCallback);
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
