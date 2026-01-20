#include "saturn/core/sys_ui.hpp"

#include "saturn/platform/AGDK/jni_helper.hpp"
#include "saturn/platform/AGDK/agdk_platform.hpp"

namespace saturn
{
namespace platform
{
namespace agdk
{

class AGDKSystemUI : public core::SystemUI::SystemUIImpl
{
   public:
    AGDKSystemUI() = default;
    ~AGDKSystemUI() override = default;

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

} // namespace agdk
} // namespace platform
} // namespace saturn
