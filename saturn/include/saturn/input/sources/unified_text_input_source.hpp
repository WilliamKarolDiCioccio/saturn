#pragma once

#include <memory>
#include <vector>

#include "saturn/input/sources/input_source.hpp"
#include "saturn/input/events.hpp"

namespace saturn
{
namespace input
{

/**
 * @brief The `UnifiedTextInputSource` class is an abstract base class for unified text input
 * sources.
 *
 * This source handles both regular text input (e.g., WM_CHAR on Win32) and IME composition
 * (e.g., WM_IME_* on Win32) in a single unified class, ensuring proper event ordering and
 * filtering to prevent duplicate text events.
 *
 * The source emits two types of events per frame:
 * - TextInputEvent: Contains ALL committed text from this frame (batched from WM_CHAR + IME
 * commits)
 * - IMEEvent: Contains the most recent composition lifecycle event (start, update, commit, end)
 *
 * Applications should:
 * 1. Poll getTextInputEvent() to get all committed text for insertion (batched per frame)
 * 2. Check isComposing() and getCurrentComposition() to render composition preview
 *
 * @note This class replaces the separate TextInputSource and IMESource classes.
 * @note Events are batched per frame - no circular buffer needed.
 */
class UnifiedTextInputSource : public InputSource
{
   protected:
    // Single events per frame (no circular buffers)
    TextInputEvent m_lastTextEvent;
    IMEEvent m_lastIMEEvent;

    // IME composition state
    bool m_isComposing;
    IMEComposition m_currentComposition;

    // Enable/disable all text input (need to prevent key events from being prematurely captured)
    bool m_textInputEnabled;

   public:
    UnifiedTextInputSource(window::Window* _window)
        : InputSource(_window),
          m_lastTextEvent(),
          m_lastIMEEvent(),
          m_isComposing(false),
          m_currentComposition(),
          m_textInputEnabled(true) {};

    virtual ~UnifiedTextInputSource() = default;

   public:
    /**
     * @brief Enable or disable all text input (both regular text and IME).
     *
     * When disabled, all text input is ignored. If currently composing, the composition
     * will be cancelled.
     *
     * @param _enabled True to enable text input, false to disable.
     */
    virtual void setEnabled(bool _enabled) = 0;

    /**
     * @brief Set the initial composition state (for recovering text field state).
     *
     * This is client-side only and does not call platform IME APIs. Use this when
     * focusing a text field that already has content to restore the composition state.
     *
     * @param _text The initial composition text (UTF-8).
     * @param _cursor The cursor position within the text.
     */
    virtual void setInitialComposition(const std::string& _text, size_t _cursor) = 0;

    /**
     * @brief Set the position of the IME candidate window.
     *
     * This should be called when the text cursor moves to ensure the IME candidate
     * list appears near the cursor.
     *
     * @param _x X coordinate in window client space.
     * @param _y Y coordinate in window client space.
     */
    virtual void setCandidateWindowPosition(int _x, int _y) = 0;

    /**
     * @brief Get the text input event for this frame.
     *
     * This contains ALL committed text from this frame (batched from WM_CHAR + IME commits).
     * The event is cleared at the start of each frame.
     *
     * @return The TextInputEvent for this frame, or empty event if no text input.
     */
    [[nodiscard]] const TextInputEvent& getTextInputEvent() const { return m_lastTextEvent; }

    /**
     * @brief Get the most recent IME event.
     *
     * This contains the most recent IME composition lifecycle event (start, update, commit, end).
     * The event is cleared at the start of each frame.
     *
     * @return The most recent IMEEvent, or default event if no IME activity this frame.
     */
    [[nodiscard]] const IMEEvent& getIMEEvent() const { return m_lastIMEEvent; }

    /**
     * @brief Check if currently composing text via IME.
     *
     * @return True if actively composing, false otherwise.
     */
    [[nodiscard]] bool isComposing() const { return m_isComposing; }

    /**
     * @brief Get the current composition state.
     *
     * This contains the in-progress composition text and cursor position. Use this
     * to render composition preview with underline/highlighting.
     *
     * @return The current IMEComposition (text and cursor position).
     */
    [[nodiscard]] const IMEComposition& getCurrentComposition() const
    {
        return m_currentComposition;
    }

    /**
     * @brief Factory method to create a platform-specific unified text input source.
     *
     * @param _window The window to create the input source for.
     * @return A unique_ptr to the platform-specific implementation, or nullptr if unsupported.
     */
    static std::unique_ptr<UnifiedTextInputSource> create(window::Window* _window);

    /**
     * @brief Process input from the platform-specific implementation.
     *
     * This method is called once per frame to:
     * 1. Clear previous frame's events
     * 2. Query pending events from platform
     * 3. Store the batched/most recent events
     */
    void processInput() override;

   protected:
    /**
     * @brief Query pending events from the platform-specific implementation.
     *
     * Platform implementations should populate the output vectors with pending
     * text input and IME events that have been buffered since the last call.
     *
     * For TextInputEvent: Should contain a SINGLE batched event with all codepoints from this
     * frame. For IMEEvent: Can contain multiple events; only the last one will be stored.
     *
     * @param _outTextEvents Vector to populate with pending TextInputEvents (should be 0 or 1
     * element).
     * @param _outIMEEvents Vector to populate with pending IMEEvents.
     */
    virtual void queryPendingEvents(std::vector<TextInputEvent>& _outTextEvents,
                                    std::vector<IMEEvent>& _outIMEEvents) = 0;
};

} // namespace input
} // namespace saturn
