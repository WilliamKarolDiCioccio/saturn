#include "saturn/core/sys_ui.hpp"

#include <emscripten.h>

namespace saturn
{
namespace platform
{
namespace emscripten
{

class EmscriptenSystemUI : public core::SystemUI::SystemUIImpl
{
   public:
    EmscriptenSystemUI() = default;
    ~EmscriptenSystemUI() override = default;

   public:
    std::optional<bool> showQuestionDialog(const std::string& _title, const std::string& _message,
                                           bool _allowCancel = false) const override;
    void showInfoDialog(const std::string& _title, const std::string& _message) const override;
    void showWarningDialog(const std::string& _title, const std::string& _message) const override;
    void showErrorDialog(const std::string& _title, const std::string& _message) const override;
    void showSoftwareKeyboard(const std::string& _text, uint32_t _selectionStart,
                              uint32_t _selectionEnd) const override;
    void hideSoftwareKeyboard() const override;
};

} // namespace emscripten
} // namespace platform
} // namespace saturn
