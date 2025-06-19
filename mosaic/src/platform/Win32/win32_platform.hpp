#pragma once

#include "mosaic/core/platform.hpp"

#include <string>
#include <vector>
#include <sstream>
#include <windows.h>
#include <shlobj.h>

namespace mosaic
{
namespace platform
{
namespace win32
{

class Win32Platform : public core::Platform
{
   public:
    Win32Platform(core::Application* _app) : core::Platform(_app) {};
    ~Win32Platform() override = default;

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

} // namespace win32
} // namespace platform
} // namespace mosaic
