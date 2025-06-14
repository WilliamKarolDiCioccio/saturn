#pragma once

#include "mosaic/window/window_system.hpp"

namespace mosaic
{
namespace platform
{
namespace agdk
{

class AGDKWindowSystem : public window::WindowSystem
{
   public:
    ~AGDKWindowSystem() override = default;

   public:
    pieces::RefResult<window::WindowSystem, std::string> initialize() override;
    void shutdown() override;

    void update() const override;
};

} // namespace agdk
} // namespace platform
} // namespace mosaic
