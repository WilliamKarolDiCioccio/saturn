#pragma once

#include "input_source.hpp"

namespace saturn
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
class SATURN_API KeyboardInputSource : public InputSource
{
   protected:
    // Events
    std::array<pieces::CircularBuffer<KeyboardKeyEvent, k_eventHistoryMaxSize>,
               c_keyboardKeys.size()>
        m_keyboardKeyEvents;

   public:
    KeyboardInputSource(window::Window* _window);
    virtual ~KeyboardInputSource() = default;

    static std::unique_ptr<KeyboardInputSource> create(window::Window* _window);

   public:
    virtual pieces::RefResult<InputSource, std::string> initialize() override = 0;
    virtual void shutdown() override = 0;

    virtual void pollDevice() override = 0;
    void processInput() override;

    [[nodiscard]] inline KeyboardKeyEvent getKeyEvent(KeyboardKey _key) const
    {
        return m_keyboardKeyEvents[static_cast<uint32_t>(_key)].front().unwrap();
    }

    [[nodiscard]] inline auto getKeyEventHistory(KeyboardKey _key) const
        -> decltype(m_keyboardKeyEvents[0].data())
    {
        return m_keyboardKeyEvents[static_cast<uint32_t>(_key)].data();
    }

   protected:
    [[nodiscard]] virtual InputAction queryKeyState(KeyboardKey _key) const = 0;
};

} // namespace input
} // namespace saturn
