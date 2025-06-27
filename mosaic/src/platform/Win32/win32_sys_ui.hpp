#include "mosaic/core/sys_ui.hpp"

#include <windows.h>

namespace mosaic
{
namespace platform
{
namespace win32
{

class Win32SystemUI : public core::SystemUI::SystemUIImpl
{
   public:
    Win32SystemUI() = default;
    ~Win32SystemUI() override = default;

   public:
    std::optional<bool> showQuestionDialog(const std::string& _title, const std::string& _message,
                                           bool _allowCancel = false) const override;
    void showInfoDialog(const std::string& _title, const std::string& _message) const override;
    void showWarningDialog(const std::string& _title, const std::string& _message) const override;
    void showErrorDialog(const std::string& _title, const std::string& _message) const override;
};

} // namespace win32
} // namespace platform
} // namespace mosaic
