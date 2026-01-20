#include "posix_sys_ui.hpp"

namespace saturn
{
namespace platform
{
namespace posix
{

std::optional<bool> POSIXSystemUI::showQuestionDialog(const std::string& _title,
                                                      const std::string& _message,
                                                      bool _allowCancel) const
{
    return std::nullopt;
}

void POSIXSystemUI::showInfoDialog(const std::string& _title, const std::string& _message) const {}

void POSIXSystemUI::showWarningDialog(const std::string& _title, const std::string& _message) const
{
}

void POSIXSystemUI::showErrorDialog(const std::string& _title, const std::string& _message) const {}

void POSIXSystemUI::showSoftwareKeyboard(const std::string& _text, uint32_t _selectionStart,
                                         uint32_t _selectionEnd) const
{
}

void POSIXSystemUI::hideSoftwareKeyboard() const {}

} // namespace posix
} // namespace platform
} // namespace saturn
