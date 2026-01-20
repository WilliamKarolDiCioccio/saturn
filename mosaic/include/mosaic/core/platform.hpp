#pragma once

#include <string>
#include <memory>

#include <pieces/core/result.hpp>

#include "saturn/defines.hpp"

#include "application.hpp"

namespace saturn
{
namespace core
{

using PlatformContextChangedEvent = std::function<void(void* _newContext)>;

/**
 * @brief Abstract base class representing the platform context.
 *
 * This class provides a generic interface for platform-specific context management.
 */
class SATURN_API PlatformContext
{
   private:
    struct Impl;

    Impl* m_impl;

   public:
    virtual ~PlatformContext() = default;

    static std::unique_ptr<PlatformContext> create();

   public:
    void registerPlatformContextChangedCallback(PlatformContextChangedEvent _callback);

   protected:
    void invokePlatformContextChangedCallbacks(void* _newContext);
};

/**
 * @brief Abstract base class representing the platform layer of the application.
 *
 * This class provides a generic interface for handling the platform-specific aspects of the
 * application lifecycle and the platform context resouces.
 *
 * Web and mobile runtimes are the primary reasons for this class, as they require specific
 * initialization, running, pausing, resuming, and shutdown behaviors that differ from traditional
 * desktop applications.
 */
class SATURN_API Platform
{
   private:
    static Platform* s_instance;

   protected:
    struct Impl;

    Impl* m_impl;

   public:
    Platform(Application* _app);
    virtual ~Platform();

    static std::unique_ptr<Platform> create(Application* _app);

    [[nodiscard]] static Platform* getInstance() { return s_instance; }

    [[nodiscard]] PlatformContext* getPlatformContext();

    [[nodiscard]] Application* getApplication();

   public:
    virtual pieces::RefResult<Platform, std::string> initialize() = 0;
    virtual pieces::RefResult<Platform, std::string> run() = 0;
    virtual void pause() = 0;
    virtual void resume() = 0;
    virtual void shutdown() = 0;
};

} // namespace core
} // namespace saturn
