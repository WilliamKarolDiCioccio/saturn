#pragma once

#include "mosaic/core/platform.hpp"

#include <string>
#include <vector>
#include <sstream>

namespace mosaic
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
} // namespace mosaic
