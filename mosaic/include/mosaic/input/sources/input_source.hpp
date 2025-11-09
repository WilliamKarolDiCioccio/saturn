#pragma once

#include <memory>
#include <stdexcept>

#include <pieces/core/result.hpp>
#include <pieces/containers/circular_buffer.hpp>

#include "mosaic/tools/logger.hpp"
#include "mosaic/input/constants.hpp"
#include "mosaic/input/events.hpp"
#include "mosaic/input/mappings.hpp"
#include "mosaic/window/window.hpp"

namespace mosaic
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
} // namespace mosaic
