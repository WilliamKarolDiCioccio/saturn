#pragma once

#include "mosaic/core/platform.hpp"

#include <jni.h>
#include <game-activity/GameActivity.h>
#include <game-text-input/gametextinput.h>
#include <game-activity/native_app_glue/android_native_app_glue.h>

namespace mosaic
{
namespace platform
{
namespace agdk
{

class AGDKPlatformContext : public core::PlatformContext
{
   private:
    GameActivity* m_activity;
    JNIEnv* m_env = nullptr;
    AAssetManager* m_assetManager;
    ANativeWindow* m_currentWindow;
    ANativeWindow* m_pendingWindow;

    bool m_windowChanged;
    bool m_surfaceDestroyed;

   public:
    AGDKPlatformContext();
    ~AGDKPlatformContext() override = default;

    // Persistent resources

    void setApp(android_app* _app);

    [[nodiscard]] GameActivity* getActivity() const { return m_activity; }
    [[nodiscard]] AAssetManager* getAssetManager() const { return m_assetManager; }

    // Thread-local resources

    [[nodiscard]] JNIEnv* getEnv() const { return m_env; }

    // Dynamic resources

    void updateWindow(ANativeWindow* _newWindow);
    [[nodiscard]] ANativeWindow* getCurrentWindow() const { return m_currentWindow; }
    [[nodiscard]] ANativeWindow* getPendingWindow() const { return m_pendingWindow; }

    [[nodiscard]] bool isWindowChanged() const { return m_windowChanged; }
    [[nodiscard]] bool isSurfaceDestroyed() const { return m_surfaceDestroyed; }

    void acknowledgeWindowChange();
};

class AGDKPlatform : public core::Platform
{
   public:
    AGDKPlatform(core::Application* _app) : Platform(_app) {};
    ~AGDKPlatform() override = default;

   public:
    pieces::RefResult<Platform, std::string> initialize() override;
    pieces::RefResult<Platform, std::string> run() override;
    void pause() override;
    void resume() override;
    void shutdown() override;

    std::optional<bool> showQuestionDialog(const std::string& _title, const std::string& _message,
                                           bool _allowCancel = false) const override;
    void showInfoDialog(const std::string& _title, const std::string& _message) const override;
    void showWarningDialog(const std::string& _title, const std::string& _message) const override;
    void showErrorDialog(const std::string& _title, const std::string& _message) const override;
};

} // namespace agdk
} // namespace platform
} // namespace mosaic
