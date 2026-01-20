#pragma once

#include "saturn/core/platform.hpp"

#include <string>
#include <vector>
#include <sstream>

namespace saturn
{
namespace platform
{
namespace posix
{

class POSIXPlatform : public core::Platform
{
   public:
    POSIXPlatform(core::Application* _app) : core::Platform(_app){};
    ~POSIXPlatform() override = default;

   public:
    pieces::RefResult<Platform, std::string> initialize() override;
    pieces::RefResult<Platform, std::string> run() override;
    void pause() override;
    void resume() override;
    void shutdown() override;
};

} // namespace posix
} // namespace platform
} // namespace saturn
