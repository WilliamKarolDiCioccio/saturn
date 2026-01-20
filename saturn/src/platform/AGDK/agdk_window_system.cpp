#include "agdk_window_system.hpp"

namespace saturn
{
namespace platform
{
namespace agdk
{

pieces::RefResult<core::System, std::string> AGDKWindowSystem::initialize()
{
    return pieces::OkRef<core::System, std::string>(*this);
}

void AGDKWindowSystem::shutdown() { destroyAllWindows(); }

pieces::RefResult<core::System, std::string> AGDKWindowSystem::update() {}

} // namespace agdk
} // namespace platform
} // namespace saturn
