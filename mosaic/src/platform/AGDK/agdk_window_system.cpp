#include "agdk_window_system.hpp"

namespace mosaic
{
namespace platform
{
namespace agdk
{

pieces::RefResult<window::WindowSystem, std::string> AGDKWindowSystem::initialize()
{
    return pieces::OkRef<window::WindowSystem, std::string>(*this);
}

void AGDKWindowSystem::shutdown() { destroyAllWindows(); }

void AGDKWindowSystem::update() const {}

} // namespace agdk
} // namespace platform
} // namespace mosaic
