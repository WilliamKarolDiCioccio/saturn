#pragma once

#include <string>

#include "mosaic/internal/defines.hpp"

namespace mosaic
{
namespace core
{

/**
 * @brief Provides abstraction for system UI interactions, such as dialogs and software keyboard.
 */
class SystemUI
{
   public:
    class SystemUIImpl
    {
       public:
        SystemUIImpl() = default;
        virtual ~SystemUIImpl() = default;

       public:
        virtual std::optional<bool> showQuestionDialog(const std::string& _title,
                                                       const std::string& _message,
                                                       bool _allowCancel = false) const = 0;
        virtual void showInfoDialog(const std::string& _title,
                                    const std::string& _message) const = 0;
        virtual void showWarningDialog(const std::string& _title,
                                       const std::string& _message) const = 0;
        virtual void showErrorDialog(const std::string& _title,
                                     const std::string& _message) const = 0;
        virtual void showSoftwareKeyboard(const std::string& _text, uint32_t _selectionStart,
                                          uint32_t _selectionEnd) const = 0;
        virtual void hideSoftwareKeyboard() const = 0;
    };

   private:
    MOSAIC_API static std::unique_ptr<SystemUIImpl> impl;

   public:
    SystemUI(const SystemUI&) = delete;
    SystemUI& operator=(const SystemUI&) = delete;
    SystemUI(SystemUI&&) = delete;
    SystemUI& operator=(SystemUI&&) = delete;

   public:
    MOSAIC_API [[nodiscard]] static std::optional<bool> showQuestionDialog(
        const std::string& _title, const std::string& _message, bool _allowCancel = false);
    MOSAIC_API static void showInfoDialog(const std::string& _title, const std::string& _message);
    MOSAIC_API static void showWarningDialog(const std::string& _title,
                                             const std::string& _message);
    MOSAIC_API static void showErrorDialog(const std::string& _title, const std::string& _message);
    MOSAIC_API static void showSoftwareKeyboard(const std::string& _text = "",
                                                uint32_t _selectionStart = 0,
                                                uint32_t _selectionEnd = 0);
    MOSAIC_API static void hideSoftwareKeyboard();
};

} // namespace core
} // namespace mosaic
