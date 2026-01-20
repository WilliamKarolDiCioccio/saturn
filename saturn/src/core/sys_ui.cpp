#include "saturn/core/sys_ui.hpp"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>

#include "saturn/defines.hpp"

#if defined(SATURN_PLATFORM_WINDOWS)
#include "platform/Win32/win32_sys_ui.hpp"
#elif defined(SATURN_PLATFORM_LINUX)
#include "platform/POSIX/posix_sys_ui.hpp"
#elif defined(SATURN_PLATFORM_EMSCRIPTEN)
#include "platform/Emscripten/emscripten_sys_ui.hpp"
#elif defined(SATURN_PLATFORM_ANDROID)
#include "platform/AGDK/agdk_sys_ui.hpp"
#endif

namespace saturn
{
namespace core
{

#if defined(SATURN_PLATFORM_WINDOWS)
std::unique_ptr<SystemUI::SystemUIImpl> SystemUI::impl =
    std::make_unique<platform::win32::Win32SystemUI>();
#elif defined(SATURN_PLATFORM_LINUX)
std::unique_ptr<SystemUI::SystemUIImpl> SystemUI::impl =
    std::make_unique<platform::posix::POSIXSystemUI>();
#elif defined(SATURN_PLATFORM_EMSCRIPTEN)
std::unique_ptr<SystemUI::SystemUIImpl> SystemUI::impl =
    std::make_unique<platform::emscripten::EmscriptenSystemUI>();
#elif defined(SATURN_PLATFORM_ANDROID)
std::unique_ptr<SystemUI::SystemUIImpl> SystemUI::impl =
    std::make_unique<platform::agdk::AGDKSystemUI>();
#endif

std::optional<bool> SystemUI::showQuestionDialog(const std::string& _title,
                                                 const std::string& _message, bool _allowCancel)
{
    return impl->showQuestionDialog(_title, _message, _allowCancel);
}

void SystemUI::showInfoDialog(const std::string& _title, const std::string& _message)
{
    impl->showInfoDialog(_title, _message);
}

void SystemUI::showWarningDialog(const std::string& _title, const std::string& _message)
{
    impl->showWarningDialog(_title, _message);
}

void SystemUI::showErrorDialog(const std::string& _title, const std::string& _message)
{
    impl->showErrorDialog(_title, _message);
}

void SystemUI::showSoftwareKeyboard(const std::string& _text, uint32_t _selectionStart,
                                    uint32_t _selectionEnd)
{
    impl->showSoftwareKeyboard(_text, _selectionStart, _selectionEnd);
}

void SystemUI::hideSoftwareKeyboard() { impl->hideSoftwareKeyboard(); }

} // namespace core
} // namespace saturn
