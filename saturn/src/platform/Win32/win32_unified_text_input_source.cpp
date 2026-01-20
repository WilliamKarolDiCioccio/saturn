#include "win32_unified_text_input_source.hpp"

#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include <pieces/utils/string.hpp>

#pragma comment(lib, "imm32.lib")

namespace saturn
{
namespace platform
{
namespace win32
{

// Static map to associate HWND with Win32UnifiedTextInputSource instances for window procedure
std::unordered_map<HWND, Win32UnifiedTextInputSource*> Win32UnifiedTextInputSource::s_sourceMap;

Win32UnifiedTextInputSource::Win32UnifiedTextInputSource(window::Window* _window)
    : input::UnifiedTextInputSource(_window),
      m_hwnd(nullptr),
      m_originalWndProc(nullptr),
      m_candidateX(0),
      m_candidateY(0),
      m_focusCallbackId(0)
{
}

pieces::RefResult<input::InputSource, std::string> Win32UnifiedTextInputSource::initialize()
{
    // Get native HWND from GLFW window
    GLFWwindow* glfwWindow = static_cast<GLFWwindow*>(m_window->getNativeHandle());
    m_hwnd = glfwGetWin32Window(glfwWindow);

    if (!m_hwnd)
    {
        return pieces::ErrRef<input::InputSource, std::string>("Failed to get Win32 window handle");
    }

    // Store this instance for the window procedure
    s_sourceMap[m_hwnd] = this;

    // Subclass the window to intercept both WM_CHAR and WM_IME_* messages
    m_originalWndProc = reinterpret_cast<WNDPROC>(
        SetWindowLongPtrW(m_hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(UnifiedWndProc)));

    if (!m_originalWndProc)
    {
        s_sourceMap.erase(m_hwnd);
        return pieces::ErrRef<input::InputSource, std::string>(
            "Failed to subclass window for unified text input handling");
    }

    // Set initial focus state
    m_isActive = glfwGetWindowAttrib(glfwWindow, GLFW_FOCUSED);

    // Register focus callback
    m_focusCallbackId = m_window->registerWindowFocusCallback(
        [this](int _focused)
        {
            m_isActive = _focused == GLFW_TRUE;
            if (!_focused && m_isComposing)
            {
                // Cancel composition when losing focus
                HIMC hImc = ImmGetContext(m_hwnd);
                if (hImc)
                {
                    ImmNotifyIME(hImc, NI_COMPOSITIONSTR, CPS_CANCEL, 0);
                    ImmReleaseContext(m_hwnd, hImc);
                }
            }
        });

    return pieces::OkRef<input::InputSource, std::string>(*this);
}

void Win32UnifiedTextInputSource::shutdown()
{
    // Unregister focus callback
    if (m_focusCallbackId != 0)
    {
        m_window->unregisterWindowFocusCallback(m_focusCallbackId);
        m_focusCallbackId = 0;
    }

    // Restore original window procedure
    if (m_originalWndProc && m_hwnd)
    {
        SetWindowLongPtrW(m_hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(m_originalWndProc));
        m_originalWndProc = nullptr;
    }

    // Remove from map
    if (m_hwnd)
    {
        s_sourceMap.erase(m_hwnd);
        m_hwnd = nullptr;
    }
}

void Win32UnifiedTextInputSource::pollDevice()
{
    // Events are collected via window procedure, no explicit polling needed
    ++m_pollCount;
}

void Win32UnifiedTextInputSource::setEnabled(bool _enabled)
{
    m_textInputEnabled = _enabled;

    if (!m_hwnd) return;

    if (_enabled)
    {
        // Re-associate default IME context
        ImmAssociateContextEx(m_hwnd, NULL, IACE_DEFAULT);
    }
    else
    {
        // Cancel any ongoing composition
        if (m_isComposing)
        {
            HIMC hImc = ImmGetContext(m_hwnd);
            if (hImc)
            {
                ImmNotifyIME(hImc, NI_COMPOSITIONSTR, CPS_CANCEL, 0);
                ImmReleaseContext(m_hwnd, hImc);
            }
        }

        // Completely disassociate IME context to prevent system IME dialog
        ImmAssociateContextEx(m_hwnd, NULL, IACE_IGNORENOCONTEXT);
    }
}

void Win32UnifiedTextInputSource::setInitialComposition(const std::string& _text, size_t _cursor)
{
    // Client-side state only (no platform API calls)
    m_currentComposition.text = _text;
    m_currentComposition.cursor = _cursor;
}

void Win32UnifiedTextInputSource::setCandidateWindowPosition(int _x, int _y)
{
    m_candidateX = _x;
    m_candidateY = _y;
    updateCandidateWindowPosition();
}

void Win32UnifiedTextInputSource::queryPendingEvents(
    std::vector<input::TextInputEvent>& _outTextEvents, std::vector<input::IMEEvent>& _outIMEEvents)
{
    std::lock_guard<std::mutex> lock(m_eventBufferMutex);

    // Batch all accumulated codepoints into a single TextInputEvent
    if (!m_codepointsBuffer.empty())
    {
        input::TextInputEvent event;
        event.codepoints = std::move(m_codepointsBuffer);
        event.text = pieces::utils::CodepointsToUtf8(event.codepoints);
        event.metadata.timestamp = std::chrono::high_resolution_clock::now();
        event.metadata.pollCount = m_pollCount;

        _outTextEvents.push_back(std::move(event));
        m_codepointsBuffer.clear();
    }

    // Move IME events
    _outIMEEvents = std::move(m_imeEventBuffer);
    m_imeEventBuffer.clear();
}

LRESULT CALLBACK Win32UnifiedTextInputSource::UnifiedWndProc(HWND hwnd, UINT msg, WPARAM wParam,
                                                             LPARAM lParam)
{
    auto it = s_sourceMap.find(hwnd);
    if (it == s_sourceMap.end())
    {
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    Win32UnifiedTextInputSource* source = it->second;

    // If inactive or disabled, pass through all messages without interception
    if (!source->m_isActive || !source->m_textInputEnabled)
    {
        return CallWindowProcW(source->m_originalWndProc, hwnd, msg, wParam, lParam);
    }

    switch (msg)
    {
        case WM_CHAR:
            source->handleWMChar(wParam);
            // Call original proc to allow default handling
            break;

        case WM_IME_STARTCOMPOSITION:
            source->handleIMEStartComposition();
            // Call original proc to allow default handling
            break;

        case WM_IME_COMPOSITION:
            source->handleIMEComposition(lParam);
            // Call original proc to allow default handling
            break;

        case WM_IME_ENDCOMPOSITION:
            source->handleIMEEndComposition();
            // Call original proc to allow default handling
            break;

        case WM_IME_SETCONTEXT:
            // Hide the default composition window (we render it ourselves)
            lParam &= ~ISC_SHOWUICOMPOSITIONWINDOW;
            break;

        default:
            break;
    }

    return CallWindowProcW(source->m_originalWndProc, hwnd, msg, wParam, lParam);
}

void Win32UnifiedTextInputSource::handleWMChar(WPARAM wParam)
{
    // Ignore WM_CHAR during active IME composition to prevent duplicates
    if (m_isComposing) return;

    wchar_t ch = static_cast<wchar_t>(wParam);

    // Ignore UTF-16 surrogate halves
    if (ch >= 0xD800 && ch <= 0xDFFF) return;

    char32_t codepoint = static_cast<char32_t>(wParam);

    // Filter control characters (except tab, newline, carriage return)
    if (codepoint < 32 && codepoint != '\t' && codepoint != '\n' && codepoint != '\r') return;

    // Accumulate codepoint (will be batched into single TextInputEvent in queryPendingEvents)
    std::lock_guard<std::mutex> lock(m_eventBufferMutex);
    m_codepointsBuffer.push_back(codepoint);
}

void Win32UnifiedTextInputSource::handleIMEStartComposition()
{
    m_isComposing = true;

    input::IMEEvent event;
    event.type = input::IMEEventType::CompositionStart;
    event.composition.text = "";
    event.composition.cursor = 0;

    std::lock_guard<std::mutex> lock(m_eventBufferMutex);
    m_imeEventBuffer.push_back(event);

    // Update candidate window position at composition start
    updateCandidateWindowPosition();
}

void Win32UnifiedTextInputSource::handleIMEComposition(LPARAM lParam)
{
    HIMC hImc = ImmGetContext(m_hwnd);
    if (!hImc) return;

    std::lock_guard<std::mutex> lock(m_eventBufferMutex);

    // Handle committed text (GCS_RESULTSTR)
    if (lParam & GCS_RESULTSTR)
    {
        std::u32string resultStr = getCompositionString(hImc, GCS_RESULTSTR);

        if (!resultStr.empty())
        {
            // Emit IMEEvent(Commit) for composition tracking
            // NOTE: Don't accumulate codepoints here, Windows will send WM_CHAR after
            // IME_ENDCOMPOSITION
            input::IMEEvent imeEvent;
            imeEvent.type = input::IMEEventType::CompositionCommit;
            imeEvent.composition.text = pieces::utils::Utf32ToUtf8(resultStr);
            imeEvent.composition.cursor = resultStr.length();
            m_imeEventBuffer.push_back(imeEvent);
        }
    }

    // Handle composition string (GCS_COMPSTR)
    if (lParam & GCS_COMPSTR)
    {
        std::u32string compStr = getCompositionString(hImc, GCS_COMPSTR);
        size_t cursor = getCursorPosition(hImc);

        input::IMEEvent event;
        event.type = input::IMEEventType::CompositionUpdate;
        event.composition.text = pieces::utils::Utf32ToUtf8(compStr);
        event.composition.cursor = cursor;
        m_imeEventBuffer.push_back(event);
    }

    ImmReleaseContext(m_hwnd, hImc);
}

void Win32UnifiedTextInputSource::handleIMEEndComposition()
{
    m_isComposing = false;

    input::IMEEvent event;
    event.type = input::IMEEventType::CompositionEnd;
    event.composition.text = "";
    event.composition.cursor = 0;

    std::lock_guard<std::mutex> lock(m_eventBufferMutex);
    m_imeEventBuffer.push_back(event);
}

std::u32string Win32UnifiedTextInputSource::getCompositionString(HIMC hImc, DWORD dwIndex)
{
    LONG byteSize = ImmGetCompositionStringW(hImc, dwIndex, nullptr, 0);
    if (byteSize <= 0) return {};

    size_t wcharCount = byteSize / sizeof(wchar_t);
    std::wstring wideStr(wcharCount + 1, L'\0');

    ImmGetCompositionStringW(hImc, dwIndex, wideStr.data(), byteSize);

    std::u32string result;
    result.reserve(wcharCount);

    for (size_t i = 0; i < wcharCount; ++i)
    {
        wchar_t ch = wideStr[i];

        if (ch >= 0xD800 && ch <= 0xDBFF && i + 1 < wcharCount)
        {
            wchar_t low = wideStr[i + 1];
            if (low >= 0xDC00 && low <= 0xDFFF)
            {
                char32_t cp = 0x10000 + ((ch - 0xD800) << 10) + (low - 0xDC00);
                result.push_back(cp);
                ++i;
                continue;
            }
        }

        if (ch >= 0xD800 && ch <= 0xDFFF)
        {
            result.push_back(0xFFFD);
        }
        else
        {
            result.push_back(static_cast<char32_t>(ch));
        }
    }

    return result;
}

size_t Win32UnifiedTextInputSource::getCursorPosition(HIMC hImc)
{
    LONG cursorPos = ImmGetCompositionStringW(hImc, GCS_CURSORPOS, nullptr, 0);
    return cursorPos >= 0 ? static_cast<size_t>(cursorPos) : 0;
}

void Win32UnifiedTextInputSource::updateCandidateWindowPosition()
{
    if (!m_hwnd) return;

    HIMC hImc = ImmGetContext(m_hwnd);
    if (!hImc) return;

    COMPOSITIONFORM cf;
    cf.dwStyle = CFS_POINT;
    cf.ptCurrentPos.x = m_candidateX;
    cf.ptCurrentPos.y = m_candidateY;
    ImmSetCompositionWindow(hImc, &cf);

    CANDIDATEFORM cand;
    cand.dwIndex = 0;
    cand.dwStyle = CFS_CANDIDATEPOS;
    cand.ptCurrentPos.x = m_candidateX;
    cand.ptCurrentPos.y = m_candidateY;
    ImmSetCandidateWindow(hImc, &cand);

    ImmReleaseContext(m_hwnd, hImc);
}

} // namespace win32
} // namespace platform
} // namespace saturn
