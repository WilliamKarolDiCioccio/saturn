#include "win32_sys_ui.hpp"

namespace mosaic
{
namespace platform
{
namespace win32
{

std::optional<bool> Win32SystemUI::showQuestionDialog(const std::string& _title,
                                                      const std::string& _message,
                                                      bool _allowCancel) const
{
    UINT flags = _allowCancel ? MB_YESNOCANCEL : MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2;

    int result = MessageBoxA(nullptr, _message.c_str(), _title.c_str(), flags);

    if (result == IDCANCEL) return std::nullopt;

    return result == IDYES;
}

void Win32SystemUI::showInfoDialog(const std::string& _title, const std::string& _message) const
{
    MessageBoxA(nullptr, _message.c_str(), _title.c_str(), MB_OK | MB_ICONINFORMATION);
}

void Win32SystemUI::showWarningDialog(const std::string& _title, const std::string& _message) const
{
    MessageBoxA(nullptr, _message.c_str(), _title.c_str(), MB_OK | MB_ICONWARNING);
}

void Win32SystemUI::showErrorDialog(const std::string& _title, const std::string& _message) const
{
    MessageBoxA(nullptr, _message.c_str(), _title.c_str(), MB_OK | MB_ICONERROR);
}

} // namespace win32
} // namespace platform
} // namespace mosaic
