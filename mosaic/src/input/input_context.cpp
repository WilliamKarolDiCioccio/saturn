#include "mosaic/input/input_context.hpp"

namespace mosaic
{
namespace input
{

pieces::RefResult<InputContext, std::string> InputContext::initialize()
{
    return pieces::OkRef<InputContext, std::string>(*this);
}

void InputContext::shutdown()
{
    removeSource<MouseInputSource>();
    removeSource<KeyboardInputSource>();
}

void InputContext::update()
{
    if (m_mouseSource) m_mouseSource->processInput();
    if (m_keyboardInputSource) m_keyboardInputSource->processInput();
    if (m_textInputSource) m_textInputSource->processInput();

    m_triggeredActionsCache.clear();
}

void InputContext::loadVirtualKeysAndButtons(const std::string& _filePath)
{
    std::ifstream file(_filePath);

    if (!file.is_open())
    {
        MOSAIC_ERROR("Failed to open virtual mouse buttons file: {}", _filePath);
        return;
    }

    nlohmann::json jsonData;

    try
    {
        file >> jsonData;
    }
    catch (const nlohmann::json::parse_error& e)
    {
        MOSAIC_ERROR("Failed to parse virtual mouse buttons JSON: {}", e.what());
        return;
    }

    if (jsonData.contains("virtualMouseButtons"))
    {
        for (const auto& [k, v] : jsonData["virtualMouseButtons"].items())
        {
            m_virtualMouseButtons[k] = static_cast<MouseButton>(v);
        }
    }

    if (jsonData.contains("virtualKeyboardKeys"))
    {
        for (const auto& [k, v] : jsonData["virtualKeyboardKeys"].items())
        {
            m_virtualKeyboardKeys[v] = static_cast<KeyboardKey>(v);
        }
    }
}

void InputContext::saveVirtualKeysAndButtons(const std::string& _filePath)
{
    std::ifstream file(_filePath);

    if (!file.is_open())
    {
        MOSAIC_ERROR("Failed to open virtual keyboard keys file: {}", _filePath);
        return;
    }

    nlohmann::json jsonData;

    try
    {
        file >> jsonData;
    }
    catch (const nlohmann::json::parse_error& e)
    {
        MOSAIC_ERROR("Failed to parse virtual keyboard keys JSON: {}", e.what());
        return;
    }
}

void InputContext::updateVirtualKeyboardKeys(
    const std::unordered_map<std::string, KeyboardKey>&& _map)
{
    const auto backupMap = m_virtualKeyboardKeys;

    try
    {
        for (const auto& [k, v] : _map)
        {
            if (m_virtualKeyboardKeys.find(k) == m_virtualKeyboardKeys.end())
            {
                MOSAIC_WARN("Virtual keyboard key not found: {}", k);
            }

            m_virtualKeyboardKeys[k] = v;
        }
    }
    catch (const std::exception& e)
    {
        m_virtualKeyboardKeys = backupMap;
        throw;
    }
}

void InputContext::updateVirtualMouseButtons(
    const std::unordered_map<std::string, MouseButton>&& _map)
{
    auto backupMap = m_virtualMouseButtons;

    try
    {
        for (const auto& [k, v] : _map)
        {
            if (m_virtualMouseButtons.find(k) == m_virtualMouseButtons.end())
            {
                MOSAIC_WARN("Virtual mouse button not found: {}", k);
            }

            m_virtualMouseButtons[k] = v;
        }
    }
    catch (const std::exception& e)
    {
        m_virtualMouseButtons = backupMap;

        MOSAIC_ERROR("Failed to update virtual mouse buttons: {}", e.what());
    }
}

void InputContext::registerActions(const std::vector<Action>&& _actions)
{
    const auto backupActions = m_actions;

    try
    {
        for (auto& action : _actions)
        {
            if (m_actions.find(action.name) != m_actions.end())
            {
                MOSAIC_ERROR("An action with the name '{}' already exists. ", action.name);
                continue;
            }

            m_actions[action.name] = action;
        }
    }
    catch (const std::exception& e)
    {
        m_actions = backupActions;

        MOSAIC_ERROR("Failed to register actions: {}", e.what());
    }
}

void InputContext::unregisterActions(const std::vector<std::string>&& _names)
{
    for (const auto& name : _names)
    {
        if (m_actions.find(name) == m_actions.end())
        {
            MOSAIC_ERROR("An action with the name '{}' does not exist. ", name);
            continue;
        }

        m_actions.erase(name);
    }
}

bool InputContext::isActionTriggered(const std::string& _name, bool _onlyCurrPoll)
{
    const auto actionIt = m_actions.find(_name);

    if (actionIt == m_actions.end())
    {
        MOSAIC_ERROR("Action not found: {}", _name);
        return false;
    }

    const auto cacheIt = m_triggeredActionsCache.find(_name);

    if (cacheIt != m_triggeredActionsCache.end())
    {
        return cacheIt->second;
    }

    auto result = actionIt->second.trigger(this);

    m_triggeredActionsCache[_name] = result;

    return result;
}

KeyboardKey InputContext::translateKey(const std::string& _key) const
{
    if (m_virtualKeyboardKeys.find(_key) == m_virtualKeyboardKeys.end())
    {
        MOSAIC_ERROR("Virtual keyboard key not found: {}", _key);
        return static_cast<KeyboardKey>(0);
    };

    return m_virtualKeyboardKeys.at(_key);
}

MouseButton InputContext::translateButton(const std::string& _button) const
{
    if (m_virtualMouseButtons.find(_button) == m_virtualMouseButtons.end())
    {
        MOSAIC_ERROR("Virtual mouse button not found: {}", _button);
        return static_cast<MouseButton>(0);
    };

    return m_virtualMouseButtons.at(_button);
}

} // namespace input
} // namespace mosaic
