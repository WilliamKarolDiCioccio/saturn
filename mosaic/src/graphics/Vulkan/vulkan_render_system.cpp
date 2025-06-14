#include "vulkan_render_system.hpp"

namespace mosaic
{
namespace graphics
{
namespace vulkan
{

pieces::RefResult<RenderSystem, std::string> VulkanRenderSystem::initialize(
    const window::Window* _window)
{
    createInstance(m_instance);

    Surface dummySurface;
    createSurface(dummySurface, m_instance, _window->getNativeHandle());

    createDevice(m_device, m_instance, dummySurface);

    destroySurface(dummySurface, m_instance);

    return pieces::OkRef<RenderSystem, std::string>(*this);
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
} // namespace mosaic
