#pragma once

#include <string>
#include <vector>
#include <GLFW/glfw3.h>
#include <emscripten.h>
#include <emscripten/html5.h>

#include "mosaic/core/platform.hpp"

namespace mosaic
{
namespace platform
{
namespace emscripten
{

class EmscriptenPlatform : public core::Platform
{
   public:
    EmscriptenPlatform(core::Application* _app) : core::Platform(_app) {};
    ~EmscriptenPlatform() override = default;

   public:
    pieces::RefResult<Platform, std::string> initialize() override;
    pieces::RefResult<Platform, std::string> run() override;
    void pause() override;
    void resume() override;
    void shutdown() override;

    std::optional<bool> showQuestionDialog(const std::string& _title, const std::string& _message,
                                           bool _allowCancel = false) const override;
    void showInfoDialog(const std::string& _title, const std::string& _message) const override;
    void showWarningDialog(const std::string& _title, const std::string& _message) const override;
    void showErrorDialog(const std::string& _title, const std::string& _message) const override;
};

} // namespace emscripten
} // namespace platform
} // namespace mosaic
