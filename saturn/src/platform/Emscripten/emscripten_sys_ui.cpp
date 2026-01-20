#include "emscripten_sys_ui.hpp"

namespace saturn
{
namespace platform
{
namespace emscripten
{

std::optional<bool> EmscriptenSystemUI::showQuestionDialog(const std::string& _title,
                                                           const std::string& _message,
                                                           [[maybe_unused]] bool _allowCancel) const
{
    // There's no native three-option dialog so allwowCancel is ignored

    int result = EM_ASM_INT(
        {
            var title = UTF8ToString($0);
            var msg = UTF8ToString($1);
            return confirm(title + "\n\n" + msg) ? 1 : 0;
        },
        _title.c_str(), _message.c_str());

    return result == 1;
}

void EmscriptenSystemUI::showInfoDialog(const std::string& _title,
                                        const std::string& _message) const
{
    EM_ASM(
        {
            var title = UTF8ToString($0);
            var msg = UTF8ToString($1);
            alert("[Info] " + title + "\n\n" + msg);
        },
        _title.c_str(), _message.c_str());
}

void EmscriptenSystemUI::showWarningDialog(const std::string& _title,
                                           const std::string& _message) const
{
    EM_ASM(
        {
            var title = UTF8ToString($0);
            var msg = UTF8ToString($1);
            alert("[Warning] " + title + "\n\n" + msg);
        },
        _title.c_str(), _message.c_str());
}

void EmscriptenSystemUI::showErrorDialog(const std::string& _title,
                                         const std::string& _message) const
{
    EM_ASM(
        {
            var title = UTF8ToString($0);
            var msg = UTF8ToString($1);
            alert("[Error] " + title + "\n\n" + msg);
        },
        _title.c_str(), _message.c_str());
}

void EmscriptenSystemUI::showSoftwareKeyboard(const std::string& _text, uint32_t _selectionStart,
                                              uint32_t _selectionEnd) const
{
}

void EmscriptenSystemUI::hideSoftwareKeyboard() const {}

} // namespace emscripten
} // namespace platform
} // namespace saturn
