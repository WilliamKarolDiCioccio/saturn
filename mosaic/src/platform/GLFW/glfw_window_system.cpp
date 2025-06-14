#include "glfw_window_system.hpp"

namespace mosaic
{
namespace platform
{
namespace glfw
{

pieces::RefResult<window::WindowSystem, std::string> GLFWWindowSystem::initialize()
{
    if (!glfwInit())
    {
        return pieces::ErrRef<window::WindowSystem, std::string>("Failed to initialize GLFW");
    }

    return pieces::OkRef<window::WindowSystem, std::string>(*this);
}

void GLFWWindowSystem::shutdown()
{
    destroyAllWindows();

    glfwTerminate();
}

void GLFWWindowSystem::update() const { glfwPollEvents(); }

} // namespace glfw
} // namespace platform
} // namespace mosaic
