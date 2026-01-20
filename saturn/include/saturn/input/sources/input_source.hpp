#pragma once

#include <memory>
#include <stdexcept>

#include <pieces/core/result.hpp>
#include <pieces/containers/circular_buffer.hpp>

#include "saturn/tools/logger.hpp"
#include "saturn/input/constants.hpp"
#include "saturn/input/events.hpp"
#include "saturn/input/mappings.hpp"
#include "saturn/window/window.hpp"

namespace saturn
{
namespace input
{

/**
 * @brief The `InputSource` class is an abstract base class for all input sources.
 *
 * Input sources provide a unified platform-agnostic interface for polling and processing input
 * events for a specific input device.
 *
 * @note This class is not meant to be instantiated directly. Instead, use derived classes such as
 * `MouseInputSource` and `KeyboardInputSource`.
 */
class InputSource
{
   protected:
    bool m_isActive;
    uint64_t m_pollCount;
    window::Window* m_window;

   public:
    InputSource(window::Window* _window) : m_isActive(false), m_pollCount(0), m_window(_window) {};
    virtual ~InputSource() = default;

   public:
    virtual pieces::RefResult<InputSource, std::string> initialize() = 0;
    virtual void shutdown() = 0;

    virtual void pollDevice() = 0;
    virtual void processInput() = 0;

    [[nodiscard]] inline bool isActive() const { return m_isActive; }
    [[nodiscard]] inline uint64_t getPollCount() const { return m_pollCount; }
};

} // namespace input
} // namespace saturn
