#pragma once

#include "action.hpp"

#include "saturn/tools/logger.hpp"
#include "sources/input_source.hpp"
#include "sources/mouse_input_source.hpp"
#include "sources/keyboard_input_source.hpp"
#include "sources/unified_text_input_source.hpp"

namespace saturn
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
class SATURN_API InputContext
{
   private:
    struct Impl;

    Impl* m_impl;

   public:
    InputContext(const window::Window* _window);
    ~InputContext();

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

#define DEFINE_SOURCE(_Type, _Member, _Name)                  \
    pieces::Result<_Type*, std::string> add##_Name##Source(); \
    void remove##_Name##Source();                             \
    [[nodiscard]] bool has##_Name##Source() const;            \
    [[nodiscard]] _Type* get##_Name##Source() const;

#include "sources.def"

#undef DEFINE_SOURCE
};

} // namespace input
} // namespace saturn
