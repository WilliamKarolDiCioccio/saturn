#include "saturn/input/input_context.hpp"

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <exception>
#include <fstream>

#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

#include <pieces/core/result.hpp>

#include "saturn/tools/logger.hpp"
#include "saturn/input/action.hpp"
#include "saturn/input/mappings.hpp"
#include "saturn/window/window.hpp"

namespace saturn
{
namespace input
{

struct InputContext::Impl
{
    const window::Window* window;

    // Input sources
#define DEFINE_SOURCE(_Type, _Member, _Name) std::unique_ptr<_Type> _Member;
#include "saturn/input/sources.def"
#undef DEFINE_SOURCE

    // Virtual keys and buttons mapped to their native equivalents
    std::unordered_map<std::string, KeyboardKey> virtualKeyboardKeys;
    std::unordered_map<std::string, MouseButton> virtualMouseButtons;

    // Bound actions triggers
    std::unordered_map<std::string, Action> actions;

    // Cache
    std::unordered_map<std::string, bool> triggeredActionsCache;

    Impl(const window::Window* _window) : window(_window) {};
};

InputContext::InputContext(const window::Window* _window) : m_impl(new Impl(_window)) {}

InputContext::~InputContext() { delete m_impl; }

pieces::RefResult<InputContext, std::string> InputContext::initialize()
{
    return pieces::OkRef<InputContext, std::string>(*this);
}

void InputContext::shutdown()
{
    removeUnifiedTextInputSource();
    removeKeyboardInputSource();
    removeMouseInputSource();
}

void InputContext::update()
{
    if (m_impl->mouseSource) m_impl->mouseSource->processInput();
    if (m_impl->keyboardInputSource) m_impl->keyboardInputSource->processInput();
    if (m_impl->unifiedTextInputSource) m_impl->unifiedTextInputSource->processInput();

    m_impl->triggeredActionsCache.clear();
}

void InputContext::loadVirtualKeysAndButtons(const std::string& _filePath)
{
    std::ifstream file(_filePath);

    if (!file.is_open())
    {
        SATURN_ERROR("Failed to open virtual mouse buttons file: {}", _filePath);
        return;
    }

    nlohmann::json jsonData;

    try
    {
        file >> jsonData;
    }
    catch (const nlohmann::json::parse_error& e)
    {
        SATURN_ERROR("Failed to parse virtual mouse buttons JSON: {}", e.what());
        return;
    }

    if (jsonData.contains("virtualMouseButtons"))
    {
        for (const auto& [k, v] : jsonData["virtualMouseButtons"].items())
        {
            m_impl->virtualMouseButtons[k] = static_cast<MouseButton>(v);
        }
    }

    if (jsonData.contains("virtualKeyboardKeys"))
    {
        for (const auto& [k, v] : jsonData["virtualKeyboardKeys"].items())
        {
            m_impl->virtualKeyboardKeys[v] = static_cast<KeyboardKey>(v);
        }
    }
}

void InputContext::saveVirtualKeysAndButtons(const std::string& _filePath)
{
    std::ifstream file(_filePath);

    if (!file.is_open())
    {
        SATURN_ERROR("Failed to open virtual keyboard keys file: {}", _filePath);
        return;
    }

    nlohmann::json jsonData;

    try
    {
        file >> jsonData;
    }
    catch (const nlohmann::json::parse_error& e)
    {
        SATURN_ERROR("Failed to parse virtual keyboard keys JSON: {}", e.what());
    }
}

void InputContext::updateVirtualKeyboardKeys(
    const std::unordered_map<std::string, KeyboardKey>&& _map)
{
    const auto backupMap = m_impl->virtualKeyboardKeys;
    auto& virtualKeyboardKeys = m_impl->virtualKeyboardKeys;

    try
    {
        for (const auto& [k, v] : _map)
        {
            if (virtualKeyboardKeys.find(k) == virtualKeyboardKeys.end())
            {
                SATURN_WARN("Virtual keyboard key not found: {}", k);
            }

            virtualKeyboardKeys[k] = v;
        }
    }
    catch (const std::exception& e)
    {
        virtualKeyboardKeys = backupMap;

        SATURN_ERROR("Failed to update virtual keyboard keys: {}", e.what());
    }
}

void InputContext::updateVirtualMouseButtons(
    const std::unordered_map<std::string, MouseButton>&& _map)
{
    auto backupMap = m_impl->virtualMouseButtons;

    try
    {
        for (const auto& [k, v] : _map)
        {
            if (m_impl->virtualMouseButtons.find(k) == m_impl->virtualMouseButtons.end())
            {
                SATURN_WARN("Virtual mouse button not found: {}", k);
            }

            m_impl->virtualMouseButtons[k] = v;
        }
    }
    catch (const std::exception& e)
    {
        m_impl->virtualMouseButtons = backupMap;

        SATURN_ERROR("Failed to update virtual mouse buttons: {}", e.what());
    }
}

void InputContext::registerActions(const std::vector<Action>&& _actions)
{
    auto& actions = m_impl->actions;

    const auto backupActions = m_impl->actions;

    try
    {
        for (auto& action : _actions)
        {
            if (actions.find(action.name) != actions.end())
            {
                SATURN_ERROR("An action with the name '{}' already exists. ", action.name);
                continue;
            }

            actions[action.name] = action;
        }
    }
    catch (const std::exception& e)
    {
        actions = backupActions;

        SATURN_ERROR("Failed to register actions: {}", e.what());
    }
}

void InputContext::unregisterActions(const std::vector<std::string>&& _names)
{
    for (const auto& name : _names)
    {
        if (m_impl->actions.find(name) == m_impl->actions.end())
        {
            SATURN_ERROR("An action with the name '{}' does not exist. ", name);
            continue;
        }

        m_impl->actions.erase(name);
    }
}

bool InputContext::isActionTriggered(const std::string& _name, bool _onlyCurrPoll)
{
    auto& actions = m_impl->actions;
    auto& triggeredActionsCache = m_impl->triggeredActionsCache;

    const auto actionIt = m_impl->actions.find(_name);

    if (actionIt == actions.end())
    {
        SATURN_ERROR("Action not found: {}", _name);
        return false;
    }

    const auto cacheIt = triggeredActionsCache.find(_name);

    if (cacheIt != triggeredActionsCache.end())
    {
        return cacheIt->second;
    }

    auto result = actionIt->second.trigger(this);

    triggeredActionsCache[_name] = result;

    return result;
}

KeyboardKey InputContext::translateKey(const std::string& _key) const
{
    auto& virtualKeyboardKeys = m_impl->virtualKeyboardKeys;

    if (virtualKeyboardKeys.find(_key) == virtualKeyboardKeys.end())
    {
        SATURN_ERROR("Virtual keyboard key not found: {}", _key);
        return static_cast<KeyboardKey>(0);
    };

    return virtualKeyboardKeys.at(_key);
}

MouseButton InputContext::translateButton(const std::string& _button) const
{
    auto& virtualMouseButtons = m_impl->virtualMouseButtons;

    if (virtualMouseButtons.find(_button) == virtualMouseButtons.end())
    {
        SATURN_ERROR("Virtual mouse button not found: {}", _button);
        return static_cast<MouseButton>(0);
    };

    return virtualMouseButtons.at(_button);
}

#define DEFINE_SOURCE(_Type, _Member, _Name)                                     \
    pieces::Result<_Type*, std::string> InputContext::add##_Name##Source()       \
    {                                                                            \
        auto& member = m_impl->_Member;                                          \
        if (member != nullptr)                                                   \
        {                                                                        \
            SATURN_WARN(#_Type " already exists.");                              \
            return pieces::Ok<_Type*, std::string>(member.get());                \
        }                                                                        \
                                                                                 \
        member = _Type::create(const_cast<window::Window*>(m_impl->window));     \
        auto result = member->initialize();                                      \
        if (result.isErr())                                                      \
        {                                                                        \
            SATURN_ERROR("Failed to initialize " #_Type ": {}", result.error()); \
            return pieces::Err<_Type*, std::string>(std::move(result.error()));  \
        }                                                                        \
                                                                                 \
        return pieces::Ok<_Type*, std::string>(member.get());                    \
    }                                                                            \
                                                                                 \
    void InputContext::remove##_Name##Source()                                   \
    {                                                                            \
        auto& member = m_impl->_Member;                                          \
        if (!member) return;                                                     \
        member->shutdown();                                                      \
        member.reset();                                                          \
    }                                                                            \
                                                                                 \
    [[nodiscard]] bool InputContext::has##_Name##Source() const                  \
    {                                                                            \
        return m_impl->_Member != nullptr;                                       \
    }                                                                            \
                                                                                 \
    [[nodiscard]] _Type* InputContext::get##_Name##Source() const { return m_impl->_Member.get(); }

#include "saturn/input/sources.def"

#undef DEFINE_SOURCE

} // namespace input
} // namespace saturn
