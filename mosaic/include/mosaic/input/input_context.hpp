#pragma once

#include <optional>
#include <fstream>
#include <nlohmann/json.hpp>

#include "action.hpp"

#include "mosaic/core/logger.hpp"
#include "sources/input_source.hpp"
#include "sources/mouse_input_source.hpp"
#include "sources/keyboard_input_source.hpp"
#include "sources/text_input_source.hpp"

namespace mosaic
{
namespace input
{

/**
 * @brief The `InputContext` class is the main interface for handling input events and actions.
 *
 * `InputContext`'s responsibilities include:
 *
 * - Managing input sources, such as mouse and keyboard, to capture input events.
 *
 * - Managing actions, which are high-level abstractions of input events.
 *
 * - Dynamic mapping and serialization/deserialization of virtual keys and buttons to their native
 * equivalents.
 *
 * - Improving performance by caching input and action states.
 *
 * @note Due to their common and limited usage some input events can also be checked outside of the
 * action system. For example, mouse cursor and wheel have dedicated getters.
 *
 * @see Action
 * @see MouseInputSource
 * @see KeyboardInputSource
 * @see TextInputSource
 */
class MOSAIC_API InputContext
{
   private:
    const window::Window* m_window;

    // Input sources
    std::unique_ptr<MouseInputSource> m_mouseSource;
    std::unique_ptr<KeyboardInputSource> m_keyboardInputSource;
    std::unique_ptr<TextInputSource> m_textInputSource;

    // Virtual keys and buttons mapped to their native equivalents
    std::unordered_map<std::string, KeyboardKey> m_virtualKeyboardKeys;
    std::unordered_map<std::string, MouseButton> m_virtualMouseButtons;

    // Bound actions triggers
    std::unordered_map<std::string, Action> m_actions;

    // Cache
    std::unordered_map<std::string, bool> m_triggeredActionsCache;

   public:
    InputContext(const window::Window* _window) : m_window(_window) {};

    InputContext(InputContext&) = delete;
    InputContext& operator=(InputContext&) = delete;
    InputContext(const InputContext&) = delete;
    InputContext& operator=(const InputContext&) = delete;

   public:
    pieces::RefResult<InputContext, std::string> initialize();
    void shutdown();

    void update();

    void loadVirtualKeysAndButtons(const std::string& _filePath);
    void saveVirtualKeysAndButtons(const std::string& _filePath);
    void updateVirtualKeyboardKeys(const std::unordered_map<std::string, KeyboardKey>&& _map);
    void updateVirtualMouseButtons(const std::unordered_map<std::string, MouseButton>&& _map);

    void registerActions(const std::vector<Action>&& _actions);
    void unregisterActions(const std::vector<std::string>&& _name);

    [[nodiscard]] bool isActionTriggered(const std::string& _name, bool _onlyCurrPoll = true);

    [[nodiscard]] KeyboardKey translateKey(const std::string& _key) const;
    [[nodiscard]] MouseButton translateButton(const std::string& _button) const;

    template <typename T>
    pieces::Result<T*, std::string> addSource()
    {
        if constexpr (std::is_same_v<T, MouseInputSource>)
        {
            if (m_mouseSource != nullptr)
            {
                MOSAIC_WARN("Mouse input source already exists.");
                return pieces::Ok<T*, std::string>(m_mouseSource.get());
            }

            m_mouseSource = MouseInputSource::create(const_cast<window::Window*>(m_window));

            auto result = m_mouseSource->initialize();

            if (result.isErr())
            {
                MOSAIC_ERROR("Failed to initialize mouse input source: {}", result.error());
                return pieces::Err<T*, std::string>(std::move(result.error()));
            }

            return pieces::Ok<T*, std::string>(m_mouseSource.get());
        }
        else if constexpr (std::is_same_v<T, KeyboardInputSource>)
        {
            if (m_keyboardInputSource != nullptr)
            {
                MOSAIC_WARN("Keyboard input source already exists.");
                return pieces::Ok<T*, std::string>(m_keyboardInputSource.get());
            }

            m_keyboardInputSource =
                KeyboardInputSource::create(const_cast<window::Window*>(m_window));

            auto result = m_keyboardInputSource->initialize();

            if (result.isErr())
            {
                MOSAIC_ERROR("Failed to initialize keyboard input source: {}", result.error());
                return pieces::Err<T*, std::string>(std::move(result.error()));
            }

            return pieces::Ok<T*, std::string>(m_keyboardInputSource.get());
        }
        else if constexpr (std::is_same_v<T, TextInputSource>)
        {
            if (m_textInputSource != nullptr)
            {
                MOSAIC_WARN("Text input source already exists.");
                return pieces::Ok<T*, std::string>(m_textInputSource.get());
            }

            m_textInputSource = TextInputSource::create(const_cast<window::Window*>(m_window));

            auto result = m_textInputSource->initialize();

            if (result.isErr())
            {
                MOSAIC_ERROR("Failed to initialize text input source: {}", result.error());
                return pieces::Err<T*, std::string>(std::move(result.error()));
            }

            return pieces::Ok<T*, std::string>(m_textInputSource.get());
        }
        else
        {
            static_assert(false, "Unsupported input source type");
        }
    }

    template <typename T>
    void removeSource()
    {
        if constexpr (std::is_same_v<T, MouseInputSource>)
        {
            if (!m_mouseSource) return;

            m_mouseSource->shutdown();
            m_mouseSource.reset();
        }
        else if constexpr (std::is_same_v<T, KeyboardInputSource>)
        {
            if (!m_keyboardInputSource) return;

            m_keyboardInputSource->shutdown();
            m_keyboardInputSource.reset();
        }
        else if constexpr (std::is_same_v<T, TextInputSource>)
        {
            if (!m_textInputSource) return;

            m_textInputSource->shutdown();
            m_textInputSource.reset();
        }
        else
        {
            static_assert(false, "Unsupported input source type");
        }
    }

    template <typename T>
    [[nodiscard]] inline bool hasSource()
    {
        if constexpr (std::is_same_v<T, MouseInputSource>)
        {
            return m_mouseSource != nullptr;
        }
        else if constexpr (std::is_same_v<T, KeyboardInputSource>)
        {
            return m_keyboardInputSource != nullptr;
        }
        else if constexpr (std::is_same_v<T, TextInputSource>)
        {
            return m_textInputSource != nullptr;
        }
        else
        {
            static_assert(false, "Unsupported input source type");
        }
    }

    template <typename T>
    [[nodiscard]] inline auto getSource() -> T*
    {
        if constexpr (std::is_same_v<T, MouseInputSource>)
        {
            return m_mouseSource.get();
        }
        else if constexpr (std::is_same_v<T, KeyboardInputSource>)
        {
            return m_keyboardInputSource.get();
        }
        else if constexpr (std::is_same_v<T, TextInputSource>)
        {
            return m_textInputSource.get();
        }
        else
        {
            static_assert(false, "Unsupported input source type");
        }
    }
};

} // namespace input
} // namespace mosaic
