#include "saturn/input/sources/unified_text_input_source.hpp"

#include "saturn/tools/logger.hpp"

#ifdef SATURN_PLATFORM_WINDOWS
#include "platform/Win32/win32_unified_text_input_source.hpp"
#endif

namespace saturn
{
namespace input
{

std::unique_ptr<UnifiedTextInputSource> UnifiedTextInputSource::create(window::Window* _window)
{
#if defined(_WIN32)
    return std::make_unique<platform::win32::Win32UnifiedTextInputSource>(_window);
#else
    SATURN_WARN("UnifiedTextInputSource not implemented for this platform");
    return nullptr;
#endif
}

void UnifiedTextInputSource::processInput()
{
    if (!isActive())
    {
        return;
    }

    pollDevice();

    // Clear previous frame's events
    m_lastTextEvent = TextInputEvent();
    m_lastIMEEvent = IMEEvent();

    // Query pending events from platform-specific implementation
    std::vector<TextInputEvent> textEvents;
    std::vector<IMEEvent> imeEvents;
    queryPendingEvents(textEvents, imeEvents);

    // Store the batched text event (should only be 0 or 1 element)
    if (!textEvents.empty()) m_lastTextEvent = textEvents.back();

    // Process IME events and update composition state
    for (const auto& event : imeEvents)
    {
        m_lastIMEEvent = event;

        // Update composition state based on event type
        switch (event.type)
        {
            case IMEEventType::CompositionStart:
                m_isComposing = true;
                m_currentComposition = event.composition;
                break;

            case IMEEventType::CompositionUpdate:
                m_currentComposition = event.composition;
                break;

            case IMEEventType::CompositionCommit:
                m_currentComposition = event.composition;
                break;

            case IMEEventType::CompositionEnd:
                m_isComposing = false;
                m_currentComposition = IMEComposition(); // Clear composition
                break;
        }
    }
}

} // namespace input
} // namespace saturn
