#pragma once

#include <fstream>
#include <nlohmann/json.hpp>

#include "action.hpp"
#include "input_arena.hpp"

namespace mosaic
{
namespace input
{

/**
 * @brief The `InputContext` class is the main interface for handling input events and actions.
 *
 * `InputContext`'s responsibilities include:
 *
 * - Registering and unregistering actions, which are high-level abstractions of input events.
 *
 * - Allowing for dyamic remapping of virtual keys and buttons to their GLFW equivalents.
 *
 * - Allowing for easy serialization and deserialization of virtual key/button mappings in JSON
 * format.
 *
 * - Improving performance by caching input and action states.
 *
 * @note Due to their common and limited usage some input events can also be checked outside of the
 * action system. For example, mouse cursor and wheel have dedicated getters.
 *
 * @see InputArena
 * @see Action
 */
class MOSAIC_API InputContext
{
   private:
    // Underlying for input processing
    std::unique_ptr<InputArena> m_arena;

    // Virtual keys and buttons mapped to their GLFW equivalents
    std::unordered_map<std::string, KeyboardKey> m_virtualKeyboardKeys;
    std::unordered_map<std::string, MouseButton> m_virtualMouseButtons;

    // Bound actions triggers and cache
    std::unordered_map<std::string, Action> m_actions;

    // Cache
    glm::vec2 m_wheelOffset;
    glm::vec2 m_averagedWheelDeltas;
    glm::vec2 m_wheelSpeed;
    glm::vec2 m_wheelAccelleration;
    glm::vec2 m_cursorPosition;
    glm::vec2 m_cursorDelta;
    glm::vec2 m_averagedCursorDeltas;
    glm::vec2 m_cursorSpeed;
    double m_cursorLinearSpeed;
    glm::vec2 m_cursorAccelleration;
    double m_cursorLinearAccelleration;
    MovementDirection m_cursorMovementDirection;
    std::unordered_map<std::string, bool> m_triggeredActionsCache;

   public:
    InputContext(window::Window* _window);

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

    void registerActions(const std::unordered_map<std::string, Action>&& _actions);
    void unregisterActions(const std::vector<std::string>&& _name);
    bool isActionTriggered(const std::string& _name);

    inline const glm::vec2 getWheelOffset() const { return m_wheelOffset; }

    inline const glm::vec2 getAveragedWheelDeltas() const { return m_averagedWheelDeltas; }

    inline const glm::vec2 getWheelSpeed() const { return m_wheelSpeed; }

    inline const glm::vec2 getWheelAccelleration() const { return m_wheelAccelleration; }

    inline const glm::vec2 getCursorPosition() const { return m_cursorPosition; }

    inline const glm::vec2 getCursorDelta() const { return m_cursorDelta; }

    inline const glm::vec2 getAveragedCursorDeltas() const { return m_averagedCursorDeltas; }

    inline const glm::vec2 getCursorSpeed() const { return m_cursorSpeed; }

    inline const double getCursorLinearSpeed() const { return m_cursorLinearSpeed; }

    inline const glm::vec2 getCursorAccelleration() const { return m_cursorAccelleration; }

    inline const double getCursorLinearAccelleration() const { return m_cursorLinearAccelleration; }

    inline const MovementDirection getCursorMovementDirection() const
    {
        return m_cursorMovementDirection;
    }
};

} // namespace input
} // namespace mosaic
