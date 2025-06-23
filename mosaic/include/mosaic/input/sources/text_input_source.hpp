#pragma once

#include "input_source.hpp"

#include <unicode/utypes.h>
#include <unicode/unistr.h>

namespace mosaic
{
namespace input
{

/**
 * @brief The `KeyboardInputSource` class is an abstract base class for keyboard input sources.
 *
 * @note This class is not meant to be instantiated directly. Instead, use the
 * `KeyboardInputSource::create()` factory method to create an instance of a concrete keyboard input
 * source implementation.
 *
 * @see InputSource
 */
class MOSAIC_API TextInputSource : public InputSource
{
   protected:
    // Events
    pieces::CircularBuffer<input::TextInputEvent, EVENT_HISTORY_MAX_SIZE> m_textEvents;

   public:
    TextInputSource(window::Window* _window);
    virtual ~TextInputSource() = default;

    static std::unique_ptr<TextInputSource> create(window::Window* _window);

   public:
    virtual pieces::RefResult<InputSource, std::string> initialize() override = 0;
    virtual void shutdown() override = 0;

    virtual void pollDevice() override = 0;
    void processInput() override;

    [[nodiscard]] inline TextInputEvent getTextInputEvent() const
    {
        return m_textEvents.front().unwrap();
    }

    [[nodiscard]] inline auto getTextInputEventHistory() const -> decltype(m_textEvents.data())
    {
        return m_textEvents.data();
    }

   protected:
    [[nodiscard]] virtual std::vector<char32_t> queryCodepoints() = 0;
};

} // namespace input
} // namespace mosaic
