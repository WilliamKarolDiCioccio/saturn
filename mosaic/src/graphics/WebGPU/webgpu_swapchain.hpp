#pragma once

#include "webgpu_common.hpp"

namespace saturn
{
namespace graphics
{
namespace webgpu
{

void configureSwapchain(WGPUAdapter _adapter, WGPUDevice _device, WGPUSurface _surface,
                        GLFWwindow* _glfwHandle, glm::uvec2 _framebufferExtent);

} // namespace webgpu
} // namespace graphics
} // namespace saturn
