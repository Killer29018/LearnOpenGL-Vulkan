#define pragma

#include <vulkan/vulkan.h>

#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>

#include <string>

#include "EventHandler.hpp"

class Window : public EventDispatcher
{
  public:
    Window();
    virtual ~Window();

    void init(const std::string& name, glm::ivec2 windowSize);

    GLFWwindow* getWindow() { return m_Window; }
    glm::ivec2 getSize() { return m_WindowSize; }

    bool shouldClose();

    void getEvents();
    void swapBuffers();

    VkSurfaceKHR getSurface(VkInstance instance);

  private:
    static void keyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void mouseCallback(GLFWwindow* window, double xpos, double ypos);
    static void mouseEnterCallback(GLFWwindow* window, int entered);

  private:
    GLFWwindow* m_Window = nullptr;
    glm::ivec2 m_WindowSize;

    bool m_FirstMouse = true;
};
