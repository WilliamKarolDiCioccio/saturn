#pragma once

#include <mutex>
#include <vector>
#include <unordered_map>

#include <Windows.h>
#include <imm.h>

#include "saturn/input/sources/unified_text_input_source.hpp"

namespace saturn
{
namespace platform
{
namespace win32
{

/**
 * @brief Win32 implementation of UnifiedTextInputSource.
 *
 * Handles both regular text input (WM_CHAR) and IME composition (WM_IME_*) via
 * window subclassing. Filters WM_CHAR during active IME composition to prevent
 * duplicate text events.
 */
class Win32UnifiedTextInputSource : public input::UnifiedTextInputSource
{
   private:
    HWND m_hwnd;
    WNDPROC m_originalWndProc;

    // Thread-safe buffers (populated by WndProc)
    std::vector<char32_t> m_codepointsBuffer; // Accumulate codepoints from WM_CHAR + IME commits
    std::vector<input::IMEEvent> m_imeEventBuffer;
    std::mutex m_eventBufferMutex;

    // Candidate window position (system widget for IME candidate list)
    int m_candidateX;
    int m_candidateY;

    // Static map to associate HWND with Win32UnifiedTextInputSource instances
    static std::unordered_map<HWND, Win32UnifiedTextInputSource*> s_sourceMap;

    size_t m_focusCallbackId;

   public:
    Win32UnifiedTextInputSource(window::Window* _window);
    ~Win32UnifiedTextInputSource() override = default;

   public:
    pieces::RefResult<input::InputSource, std::string> initialize() override;
    void shutdown() override;
    void pollDevice() override;

    void setEnabled(bool _enabled) override;
    void setInitialComposition(const std::string& _text, size_t _cursor) override;
    void setCandidateWindowPosition(int _x, int _y) override;

   protected:
    void queryPendingEvents(std::vector<input::TextInputEvent>& _outTextEvents,
                            std::vector<input::IMEEvent>& _outIMEEvents) override;

   private:
    static LRESULT CALLBACK UnifiedWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    void handleWMChar(WPARAM wParam);
    void handleIMEStartComposition();
    void handleIMEComposition(LPARAM lParam);
    void handleIMEEndComposition();

    std::u32string getCompositionString(HIMC hImc, DWORD dwIndex);
    size_t getCursorPosition(HIMC hImc);
    void updateCandidateWindowPosition();
};

} // namespace win32
} // namespace platform
} // namespace saturn
