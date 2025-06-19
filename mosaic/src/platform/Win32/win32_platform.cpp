#include "win32_platform.hpp"

#include <GLFW/glfw3.h>
#include <VersionHelpers.h>

#pragma comment(lib, "Shell32.lib")
#pragma comment(lib, "Ole32.lib")

namespace mosaic
{
namespace platform
{
namespace win32
{

pieces::RefResult<core::Platform, std::string> Win32Platform::initialize()
{
    auto result = m_app->initialize();

    if (result.isErr())
    {
        return pieces::ErrRef<core::Platform, std::string>(std::move(result.error()));
    }

    m_app->resume();

    return pieces::OkRef<Platform, std::string>(*this);
}

pieces::RefResult<core::Platform, std::string> Win32Platform::run()
{
    while (!m_app->shouldExit())
    {
        if (m_app->isResumed())
        {
            auto result = m_app->update();

            if (result.isErr())
            {
                return pieces::ErrRef<core::Platform, std::string>(std::move(result.error()));
            }
        }
    }

    return pieces::OkRef<core::Platform, std::string>(*this);
}

void Win32Platform::pause() { m_app->pause(); }

void Win32Platform::resume() { m_app->resume(); }

void Win32Platform::shutdown() { m_app->shutdown(); }

std::optional<bool> Win32Platform::showQuestionDialog(const std::string& _title,
                                                      const std::string& _message,
                                                      bool _allowCancel) const
{
    UINT flags = _allowCancel ? MB_YESNOCANCEL : MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2;

    int result = MessageBoxA(nullptr, _message.c_str(), _title.c_str(), flags);

    if (result == IDCANCEL) return std::nullopt;

    return result == IDYES;
}

void Win32Platform::showInfoDialog(const std::string& _title, const std::string& _message) const
{
    MessageBoxA(nullptr, _message.c_str(), _title.c_str(), MB_OK | MB_ICONINFORMATION);
}

void Win32Platform::showWarningDialog(const std::string& _title, const std::string& _message) const
{
    MessageBoxA(nullptr, _message.c_str(), _title.c_str(), MB_OK | MB_ICONWARNING);
}

void Win32Platform::showErrorDialog(const std::string& _title, const std::string& _message) const
{
    MessageBoxA(nullptr, _message.c_str(), _title.c_str(), MB_OK | MB_ICONERROR);
}

} // namespace win32
} // namespace platform
} // namespace mosaic
