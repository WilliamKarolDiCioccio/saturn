#pragma once

#include <pieces/core/result.hpp>
#include <pieces/utils/enum_flags.hpp>

namespace saturn
{
namespace core
{

/**
 * @brief Represents the current state of a system in its lifecycle
 *
 * Defines the possible states a system can be in, from creation through shutdown.
 * This enum ensures all systems follow a consistent state management pattern.
 */
enum class SystemState
{
    uninitialized,
    initialized,
    resumed,
    paused,
    shutdown
};

/**
 * @brief Base interface that all systems in Saturn must implement
 *
 * This abstract class defines the core contract that every system must follow,
 * providing a unified interface for system lifecycle management. All systems
 * must implement initialization, update, pause/resume, and shutdown functionality.
 *
 * The system maintains its current state internally and provides query methods
 * to check the current lifecycle state.
 */
class System
{
   protected:
    SystemState m_state = SystemState::uninitialized;

   public:
    virtual ~System() = default;

   public:
    virtual pieces::RefResult<System, std::string> initialize() = 0;
    virtual pieces::RefResult<System, std::string> update() = 0;
    virtual void pause() = 0;
    virtual void resume() = 0;
    virtual void shutdown() = 0;

    [[nodiscard]] SystemState getState() const { return m_state; }
    [[nodiscard]] bool isInitialized() const { return m_state != SystemState::uninitialized; }
    [[nodiscard]] bool isPaused() const { return m_state == SystemState::paused; }
    [[nodiscard]] bool isResumed() const { return m_state == SystemState::resumed; }
};

/**
 * @brief Enumeration of engine system types
 *
 * Defines the different types of core engine systems that are managed
 * internally by Saturn. These systems provide fundamental engine functionality.
 */
enum class EngineSystemType
{
    window,
    input,
    render,
    scene,
};

/**
 * @brief Base class for internal engine systems
 *
 * Extends the System interface specifically for engine-managed systems.
 * Engine systems are internal to Saturn and provide core functionality
 * like window management, input handling, and rendering. These systems
 * have default implementations for pause/resume as they typically don't
 * need custom pause behavior.
 */
class EngineSystem : public System
{
   protected:
    EngineSystemType m_type;

   public:
    EngineSystem(EngineSystemType _type) : m_type(_type) {}
    virtual ~EngineSystem() override = default;

   public:
    virtual pieces::RefResult<System, std::string> initialize() override = 0;
    virtual pieces::RefResult<System, std::string> update() override = 0;
    void pause() override {};
    void resume() override {};
    virtual void shutdown() override = 0;

    [[nodiscard]] EngineSystemType getType() const { return m_type; }
};

/**
 * @brief Base class for client systems
 *
 * Extends the System interface for game and application-specific systems.
 * Client systems are user-defined and must provide full implementations
 * of all lifecycle methods, including custom pause and resume behavior
 * as needed by the specific application or game logic.
 */
class ClientSystem : public System
{
   public:
    virtual ~ClientSystem() override = default;

   public:
    virtual pieces::RefResult<System, std::string> initialize() override = 0;
    virtual pieces::RefResult<System, std::string> update() override = 0;
    virtual void pause() override = 0;
    virtual void resume() override = 0;
    virtual void shutdown() override = 0;
};

} // namespace core
} // namespace saturn
