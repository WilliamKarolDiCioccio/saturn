#include "mosaic/input/input_context.hpp"

namespace mosaic
{
namespace input
{

InputContext::InputContext(window::Window* _window)
    : m_arena(std::make_unique<InputArena>(_window)),
      m_wheelOffset(0.f),
      m_averagedWheelDeltas(0.f),
      m_wheelSpeed(0.f),
      m_wheelAccelleration(0.f),
      m_cursorPosition(0.f),
      m_cursorDelta(0.f),
      m_averagedCursorDeltas(0.f),
      m_cursorSpeed(0.f),
      m_cursorLinearSpeed(0.f),
      m_cursorAccelleration(0.f),
      m_cursorLinearAccelleration(0.f),
      m_cursorMovementDirection(MovementDirection::none) {};

pieces::RefResult<InputContext, std::string> InputContext::initialize()
{
    auto result = m_arena->initialize();

    if (result.isErr())
    {
        return pieces::ErrRef<InputContext, std::string>(std::move(result.error()));
    }

    return pieces::OkRef<InputContext, std::string>(*this);
}

void InputContext::shutdown() { m_arena->shutdown(); }

void InputContext::update()
{
    m_arena->update();

    m_triggeredActionsCache.clear();

    m_wheelOffset = m_arena->getWheelOffset();
    m_averagedWheelDeltas = m_arena->getAveragedWheelDeltas();
    m_wheelSpeed = m_arena->getWheelSpeed();
    m_wheelAccelleration = m_arena->getWheelAccelleration();
    m_cursorDelta = m_arena->getCursorDelta();
    m_averagedCursorDeltas = m_arena->getAveragedCursorDeltas();
    m_cursorPosition = m_arena->getCursorPosition();
    m_cursorSpeed = m_arena->getCursorSpeed();
    m_cursorLinearSpeed = m_arena->getCursorLinearSpeed();
    m_cursorAccelleration = m_arena->getCursorAccelleration();
    m_cursorLinearAccelleration = m_arena->getCursorLinearAccelleration();
    m_cursorMovementDirection = m_arena->getCursorMovementDirection();
}

void InputContext::loadVirtualKeysAndButtons(const std::string& _filePath)
{
    std::ifstream file(_filePath);

    if (!file.is_open())
    {
        MOSAIC_ERROR("Failed to open virtual mouse buttons file: {}", _filePath.c_str());
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
        MOSAIC_ERROR("Failed to open virtual keyboard keys file: {}", _filePath.c_str());
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
                MOSAIC_WARN("Virtual keyboard key not found: {}", k.c_str());
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
                MOSAIC_WARN("Virtual mouse button not found: {}", k.c_str());
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

void InputContext::registerActions(const std::unordered_map<std::string, Action>&& _actions)
{
    const auto backupActions = m_actions;

    try
    {
        for (const auto& [k, v] : _actions)
        {
            if (m_actions.find(k) != m_actions.end())
            {
                MOSAIC_WARN("Action already registered: {}", k.c_str());
                continue;
            }

            m_actions[k] = v;
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
            MOSAIC_WARN("Action not found: {}", name.c_str());
            continue;
        }

        m_actions.erase(name);
    }
}

bool InputContext::isActionTriggered(const std::string& _name)
{
    const auto translateKey = [this](const std::string& _key) -> KeyboardKey
    {
        if (m_virtualKeyboardKeys.find(_key) != m_virtualKeyboardKeys.end())
        {
            return m_virtualKeyboardKeys.at(_key);
        };

        return static_cast<KeyboardKey>(0);
    };

    const auto translateButton = [this](const std::string& _button) -> MouseButton
    {
        if (m_virtualMouseButtons.find(_button) != m_virtualMouseButtons.end())
        {
            return m_virtualMouseButtons.at(_button);
        };

        return static_cast<MouseButton>(0);
    };

    if (m_actions.find(_name) == m_actions.end() || m_actions.at(_name).empty())
    {
        MOSAIC_ERROR("Action not found: {}", _name.c_str());
        return false;
    }

    if (m_triggeredActionsCache.find(_name) != m_triggeredActionsCache.end())
    {
        return m_triggeredActionsCache.at(_name);
    }

    bool allTriggersActive = true;

    for (const auto& trigger : m_actions.at(_name))
    {
        if (std::holds_alternative<KeyboardKeyActionTrigger>(trigger))
        {
            const auto& keyTrigger = std::get<KeyboardKeyActionTrigger>(trigger);

            std::unordered_map<std::string, KeyboardKeyEvent> requiredKeysEvents;
            requiredKeysEvents.reserve(keyTrigger.requiredVirtualKeys.size());

            for (const auto& requiredKey : keyTrigger.requiredVirtualKeys)
            {
                requiredKeysEvents[requiredKey] =
                    m_arena->getKeyboardKeyEvent(translateKey(requiredKey));
            }

            allTriggersActive = keyTrigger.callback(requiredKeysEvents);
        }
        else if (std::holds_alternative<MouseButtonActionTrigger>(trigger))
        {
            const auto& mouseButtonTrigger = std::get<MouseButtonActionTrigger>(trigger);

            std::unordered_map<std::string, MouseButtonEvent> requiredButtonEvents;
            requiredButtonEvents.reserve(mouseButtonTrigger.requiredVirtualKeys.size());

            for (const auto& requiredKey : mouseButtonTrigger.requiredVirtualKeys)
            {
                requiredButtonEvents[requiredKey] =
                    m_arena->getMouseButtonEvent(translateButton(requiredKey));
            }

            allTriggersActive = mouseButtonTrigger.callback(requiredButtonEvents);
        }
        else if (std::holds_alternative<MouseCursorPosActionTrigger>(trigger))
        {
            const auto& mouseCursorPosTrigger = std::get<MouseCursorPosActionTrigger>(trigger);

            allTriggersActive = mouseCursorPosTrigger.callback(m_arena->getMouseCursorPosEvent());
        }
        else if (std::holds_alternative<MouseWheelScrollActionTrigger>(trigger))
        {
            const auto& mouseWheelScrollTrigger = std::get<MouseWheelScrollActionTrigger>(trigger);

            allTriggersActive =
                mouseWheelScrollTrigger.callback(m_arena->getMouseWheelScrollEvent());
        }
        else
        {
            MOSAIC_ERROR("Unknown action trigger type: {}", typeid(trigger).name());
        }

        if (!allTriggersActive)
        {
            break;
        }
    }

    if (allTriggersActive)
    {
        return m_triggeredActionsCache[_name] = true;
    }

    return false;
}

} // namespace input
} // namespace mosaic
