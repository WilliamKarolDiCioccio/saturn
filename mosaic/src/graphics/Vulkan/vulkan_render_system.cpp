#include "vulkan_render_system.hpp"

#include "saturn/window/window_system.hpp"

namespace saturn
{
namespace graphics
{
namespace vulkan
{

pieces::RefResult<core::System, std::string> VulkanRenderSystem::initialize()
{
    createInstance(m_instance);

    auto windowSystem = window::WindowSystem::getInstance();

    if (!windowSystem)
    {
        return pieces::ErrRef<core::System, std::string>(
            "Window system is not initialized. Cannot create Vulkan render system.");
    }

    auto window = windowSystem->getWindow("MainWindow");

    if (!window)
    {
        return pieces::ErrRef<core::System, std::string>(
            "Main window does not exist. Cannot create Vulkan render system.");
    }

    Surface dummySurface;
    createSurface(dummySurface, m_instance, window->getNativeHandle());

    createDevice(m_device, m_instance, dummySurface);

    destroySurface(dummySurface, m_instance);

    return pieces::OkRef<core::System, std::string>(*this);
}

void VulkanRenderSystem::shutdown()
{
    vkDeviceWaitIdle(m_device.device);

    destroyAllContexts();

    destroyDevice(m_device);
    destroyInstance(m_instance);
}

} // namespace vulkan
} // namespace graphics
} // namespace saturn
