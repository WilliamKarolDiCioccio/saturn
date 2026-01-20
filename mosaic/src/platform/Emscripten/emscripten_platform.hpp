#pragma once

#include <string>
#include <vector>
#include <GLFW/glfw3.h>
#include <emscripten.h>
#include <emscripten/html5.h>

#include "saturn/core/platform.hpp"

namespace saturn
{
namespace platform
{
namespace emscripten
{

class EmscriptenPlatform : public core::Platform
{
   public:
    EmscriptenPlatform(core::Application* _app) : core::Platform(_app){};
    ~EmscriptenPlatform() override = default;

   public:
    pieces::RefResult<Platform, std::string> initialize() override;
    pieces::RefResult<Platform, std::string> run() override;
    void pause() override;
    void resume() override;
    void shutdown() override;
};

} // namespace emscripten
} // namespace platform
} // namespace saturn
