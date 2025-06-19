#pragma once

#include <string>
#include <memory>
#include <vector>

#include <pieces/result.hpp>

#include "mosaic/defines.hpp"

#include "application.hpp"

namespace mosaic
{
namespace core
{

/**
 * @brief Abstract base class representing the platform context.
 *
 * This class provides a generic interface for platform-specific context management.
 */
class MOSAIC_API PlatformContext
{
   public:
    using PlatformContextChangedEvent = std::function<void(void* _newContext)>;

   private:
    std::vector<PlatformContextChangedEvent> m_platformContextListeners;

   public:
    virtual ~PlatformContext() = default;

    static std::unique_ptr<PlatformContext> create();

   public:
    inline void registerPlatformContextChangedCallback(PlatformContextChangedEvent _callback)
    {
        m_platformContextListeners.push_back(_callback);
    }

   protected:
    void invokePlatformContextChangedCallbacks(void* _context);
};

/**
 * @brief Abstract base class representing the platform layer of the application.
 *
 * This class provides a generic interface for managing the interaction between the platform and the
 * application. Specifically, it handles the lifecycle and resources and enforces platform-specific
 * behaviors.
 *
 * Web and mobile runtimes are the primary reasons for this class, as they require specific
 * initialization, running, pausing, resuming, and shutdown behaviors that differ from traditional
 * desktop applications.
 */
class MOSAIC_API Platform
{
   private:
    static Platform* s_instance;

   protected:
    Application* m_app;
    std::unique_ptr<PlatformContext> m_platformContext;

   public:
    Platform(Application* _app);
    virtual ~Platform() = default;

    static std::unique_ptr<Platform> create(Application* _app);

    [[nodiscard]] static Platform* getInstance() { return s_instance; }

    [[nodiscard]] PlatformContext* getPlatformContext() { return m_platformContext.get(); }

    virtual std::optional<bool> showQuestionDialog(const std::string& _title,
                                                   const std::string& _message,
                                                   bool _allowCancel = false) const = 0;
    virtual void showInfoDialog(const std::string& _title, const std::string& _message) const = 0;
    virtual void showWarningDialog(const std::string& _title,
                                   const std::string& _message) const = 0;
    virtual void showErrorDialog(const std::string& _title, const std::string& _message) const = 0;

   public:
    virtual pieces::RefResult<Platform, std::string> initialize() = 0;
    virtual pieces::RefResult<Platform, std::string> run() = 0;
    virtual void pause() = 0;
    virtual void resume() = 0;
    virtual void shutdown() = 0;
};

} // namespace core
} // namespace mosaic
