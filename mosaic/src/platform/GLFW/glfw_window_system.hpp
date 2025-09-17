#pragma once

#include "mosaic/window/window_system.hpp"

#include <GLFW/glfw3.h>

namespace mosaic
{
namespace platform
{
namespace glfw
{

class GLFWWindowSystem : public window::WindowSystem
{
   public:
    ~GLFWWindowSystem() override = default;

   public:
    pieces::RefResult<core::System, std::string> initialize() override;
    void shutdown() override;

    pieces::RefResult<core::System, std::string> update()  override;
};

} // namespace glfw
} // namespace platform
} // namespace mosaic
