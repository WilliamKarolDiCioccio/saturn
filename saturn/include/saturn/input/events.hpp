#pragma once

#include <string>
#include <chrono>

#include <glm/vec2.hpp>
#include <glm/geometric.hpp>

#include <pieces/utils/enum_flags.hpp>

namespace saturn
{
namespace input
{

/**
 * @brief The `InputEventType` enum class defines the types of input events that can be generated.
 */
enum class InputEventType : uint32_t
{
    none = 0,
    start = 1 << 0,
    end = 1 << 0,
    standalone = 1 << 2,
};

SATURN_DEFINE_ENUM_FLAGS_OPERATORS(InputEventType)

/**
 * @brief The `ActionableState` enum class defines the possible states for buttons, keys, or other
 * actionable input elements. It uses bit flags to allow combinations of states.
 */
enum class ActionableState : uint32_t
{
    none = 0,
    release = 1 << 0,
    press = 1 << 1,
    hold = 1 << 2,
    double_press = 1 << 3
};

SATURN_DEFINE_ENUM_FLAGS_OPERATORS(ActionableState)

/**
 * @brief The `GestureType` enum class defines the types of gestures that can be recognized. It uses
 * bit flags to allow combinations of gesture types.
 */
enum class GestureType : uint32_t
{
    none = 0,
    pan = 1 << 1,
    pinch = 1 << 2,
    rotate = 1 << 3,
};

SATURN_DEFINE_ENUM_FLAGS_OPERATORS(GestureType)

/**
 * @brief The `GestureDirection` enum class defines the possible movement directions for mouse and
 * touch drag events. It uses bit flags to allow combinations of directions.
 */
enum GestureDirection : uint32_t
{
    none = 0,
    up = 1 << 0,
    down = 1 << 1,
    left = 1 << 2,
    right = 1 << 3
};

SATURN_DEFINE_ENUM_FLAGS_OPERATORS(GestureDirection)

/**
 * @brief The `InputEventMetadata` struct contains meta-data for input events.
 */
struct InputEventMetadata
{
    std::chrono::time_point<std::chrono::high_resolution_clock> timestamp;
    std::chrono::duration<double> duration;
    uint64_t pollCount;
    InputEventType type;

    InputEventMetadata(std::chrono::time_point<std::chrono::high_resolution_clock> _now,
                       std::chrono::duration<double> _duration, uint64_t _pollCount = 0,
                       InputEventType _type = InputEventType::standalone)
        : timestamp(_now), duration(_duration), pollCount(_pollCount), type(_type) {};
};

/**
 * @brief The `MouseButtonEvent` struct contains data and meta-data for mouse button events.
 *
 * This event will be fired when a mouse button shifts from one state to another of those defined in
 * `ActionableState`.
 *
 * @see ActionableState
 */
struct MouseButtonEvent
{
    InputEventMetadata metadata;
    ActionableState state;

    MouseButtonEvent()
        : metadata(std::chrono::high_resolution_clock::now(), std::chrono::milliseconds(0), 0,
                   InputEventType::standalone),
          state(ActionableState::none) {};

    MouseButtonEvent(ActionableState _state,
                     std::chrono::time_point<std::chrono::high_resolution_clock> _now,
                     uint64_t _pollCount,
                     std::chrono::duration<double> _duration = std::chrono::milliseconds(0))
        : metadata(_now, _duration, _pollCount), state(_state) {};
};

/**
 * @brief The `MouseCursorMoveEnd` struct contains data and meta-data for mouse cursor movement end.
 *
 * This event will be fired when a mouse cursor move operations starts or ends, which is defined as
 * a change in the cursor's position by a certain threshold in a certain duration.
 */
struct MouseCursorMoveEvent
{
    InputEventMetadata metadata;
    glm::vec2 position;
    glm::vec2 delta;

    MouseCursorMoveEvent()
        : metadata(std::chrono::high_resolution_clock::now(), std::chrono::milliseconds(0), 0),
          position(0.0f),
          delta(0.0f) {};

    MouseCursorMoveEvent(glm::vec2 _position, glm::vec2 _delta,
                         std::chrono::time_point<std::chrono::high_resolution_clock> _now,
                         uint64_t _pollCount, InputEventType _type,
                         std::chrono::duration<double> _duration = std::chrono::milliseconds(0))
        : metadata(_now, _duration, _pollCount, _type), position(_position), delta(_delta) {};
};

/**
 * @brief The `MouseWheelScrollEnd` struct contains data and meta-data for mouse wheel scroll end
 * events.
 *
 * This event will be fired when a mouse wheel scroll operation starts or ends, which is defined as
 * a change in the scroll offset by a certain threshold in a certain duration.
 */
struct MouseWheelScrollEvent
{
    InputEventMetadata metadata;
    glm::vec2 offset;
    glm::vec2 delta;

    MouseWheelScrollEvent()
        : metadata(std::chrono::high_resolution_clock::now(), std::chrono::milliseconds(0), 0),
          offset(0.0f),
          delta(0.0f) {};

    MouseWheelScrollEvent(glm::vec2 _offset, glm::vec2 _delta,
                          std::chrono::time_point<std::chrono::high_resolution_clock> _now,
                          uint64_t _pollCount, InputEventType _type,
                          std::chrono::duration<double> _duration = std::chrono::milliseconds(0))
        : metadata(_now, _duration, _pollCount, _type), offset(_offset), delta(_delta) {};
};

/**
 * @brief The `KeyboardKeyEvent` struct contains data and meta-data for keyboard key events.
 */
struct KeyboardKeyEvent
{
    InputEventMetadata metadata;
    ActionableState state;

    KeyboardKeyEvent()
        : metadata(std::chrono::high_resolution_clock::now(), std::chrono::milliseconds(0), 0,
                   InputEventType::standalone),
          state(ActionableState::none) {};

    KeyboardKeyEvent(ActionableState _state,
                     std::chrono::time_point<std::chrono::high_resolution_clock> _now,
                     uint64_t _pollCount,
                     std::chrono::duration<double> _duration = std::chrono::milliseconds(0))
        : metadata(_now, _duration, _pollCount), state(_state) {};
};

/**
 * @brief The `TextInputEvent` struct contains data and meta-data for a batched text input event.
 *
 * This event is emitted once per frame when text input has occurred, and may contain multiple
 * codepoints (e.g., due to IME composition or key repeat). The `text` field is UTF-8 encoded.
 */
struct TextInputEvent
{
    InputEventMetadata metadata;

    // Raw codepoints (UCS-4)
    std::vector<char32_t> codepoints;

    // UTF-8 encoded string (suitable for display/UI)
    std::string text;

    TextInputEvent()
        : metadata(std::chrono::high_resolution_clock::now(), std::chrono::milliseconds(0), 0,
                   InputEventType::standalone),
          codepoints(),
          text() {};

    TextInputEvent(std::vector<char32_t> _codepoints, std::string _text,
                   std::chrono::time_point<std::chrono::high_resolution_clock> _now,
                   uint64_t _pollCount,
                   std::chrono::duration<double> _duration = std::chrono::milliseconds(0))
        : metadata(_now, _duration, _pollCount, InputEventType::standalone),
          codepoints(std::move(_codepoints)),
          text(std::move(_text)) {};
};

/**
 * @brief The `IMEEventType` enum class defines the types of Input Method Editor (IME) events.
 *
 * This enum is used to categorize the different stages of text composition when using an IME.
 */
enum class IMEEventType
{
    CompositionStart,
    CompositionUpdate,
    CompositionCommit,
    CompositionEnd
};

/**
 * @brief The `IMEComposition` struct contains data for Input Method Editor (IME) compositions.
 *
 * This struct holds the current composition text and the cursor position within that text.
 */
struct IMEComposition
{
    std::string text;
    size_t cursor;

    IMEComposition() : text(""), cursor(0) {};
};

/**
 * @brief The `IMEEvent` struct contains data for Input Method Editor (IME) events.
 *
 * This event is used to handle complex text input scenarios, such as those involving character
 * composition in languages like Chinese, Japanese, and Korean.
 */
struct IMEEvent
{
    IMEEventType type;
    IMEComposition composition;

    IMEEvent() : type(IMEEventType::CompositionStart), composition() {};

    IMEEvent(IMEEventType _type, const IMEComposition& _composition)
        : type(_type), composition(_composition) {};
};

} // namespace input
} // namespace saturn
