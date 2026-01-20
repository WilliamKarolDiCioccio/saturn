#pragma once

#include "saturn/core/platform.hpp"

#include <string>
#include <vector>
#include <sstream>
#include <windows.h>
#include <shlobj.h>

namespace saturn
{
namespace platform
{
namespace win32
{

class Win32Platform : public core::Platform
{
   public:
    Win32Platform(core::Application* _app) : core::Platform(_app){};
    ~Win32Platform() override = default;

   public:
    pieces::RefResult<Platform, std::string> initialize() override;
    pieces::RefResult<Platform, std::string> run() override;
    void pause() override;
    void resume() override;
    void shutdown() override;
};

} // namespace win32
} // namespace platform
} // namespace saturn
