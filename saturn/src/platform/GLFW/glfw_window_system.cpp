#include "glfw_window_system.hpp"

namespace saturn
{
namespace platform
{
namespace glfw
{

pieces::RefResult<core::System, std::string> GLFWWindowSystem::initialize()
{
    if (!glfwInit())
    {
        return pieces::ErrRef<core::System, std::string>("Failed to initialize GLFW");
    }

    return pieces::OkRef<core::System, std::string>(*this);
}

void GLFWWindowSystem::shutdown()
{
    destroyAllWindows();

    glfwTerminate();
}

pieces::RefResult<core::System, std::string> GLFWWindowSystem::update()
{
    glfwPollEvents();

    return pieces::OkRef<core::System, std::string>(*this);
}

} // namespace glfw
} // namespace platform
} // namespace saturn
