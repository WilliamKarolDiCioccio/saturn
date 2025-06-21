#include "mosaic/platform/AGDK/agdk_platform.hpp"

namespace mosaic
{
namespace platform
{
namespace agdk
{

AGDKPlatformContext::AGDKPlatformContext()
    : m_activity(nullptr),
      m_assetManager(nullptr),
      m_currentWindow(nullptr),
      m_pendingWindow(nullptr),
      m_windowChanged(false),
      m_surfaceDestroyed(false) {};

void AGDKPlatformContext::setApp(android_app* _app)
{
    if (_app)
    {
        m_activity = _app->activity;
        m_assetManager = _app->activity->assetManager;
        m_currentWindow = _app->window;
    }
}

void AGDKPlatformContext::updateWindow(ANativeWindow* _newWindow)
{
    // Store the new window as pending
    m_pendingWindow = _newWindow;

    // Check if the window actually changed
    if (m_currentWindow != _newWindow)
    {
        m_windowChanged = true;

        // Handle surface destruction
        if (_newWindow == nullptr && m_currentWindow != nullptr)
        {
            m_surfaceDestroyed = true;

            MOSAIC_DEBUG("AGDKPlatformContext: Surface destroyed");
        }
        else
        {
            m_surfaceDestroyed = false;

            MOSAIC_DEBUG("AGDKPlatformContext: Surface updated");
        }

        // If the new window is valid, acquire it
        if (_newWindow != nullptr)
        {
            ANativeWindow_acquire(_newWindow);
        }

        // Release the old window if it exists
        if (m_currentWindow != nullptr)
        {
            ANativeWindow_release(m_currentWindow);
        }

        // Update current window
        m_currentWindow = _newWindow;
    }
}

void AGDKPlatformContext::acknowledgeWindowChange()
{
    invokePlatformContextChangedCallbacks(this);

    m_windowChanged = false;
    m_pendingWindow = nullptr;
}

pieces::RefResult<core::Platform, std::string> AGDKPlatform::initialize()
{
    if (!getPlatformContext())
    {
        return pieces::ErrRef<core::Platform, std::string>("Platform context not available");
    }

    auto result = m_app->initialize();

    if (result.isErr())
    {
        return pieces::ErrRef<core::Platform, std::string>(std::move(result.error()));
    }

    return pieces::OkRef<core::Platform, std::string>(*this);
}

pieces::RefResult<core::Platform, std::string> AGDKPlatform::run()
{
    auto context = static_cast<AGDKPlatformContext*>(getPlatformContext());

    if (m_app->shouldExit())
    {
        if (context && context->getActivity())
        {
            GameActivity_finish(context->getActivity());
        }
        else
        {
            throw std::runtime_error("AGDKPlatform: Cannot run - no valid activity");
        }

        return pieces::OkRef<core::Platform, std::string>(*this);
    }

    if (m_app->isResumed() && context && context->getCurrentWindow())
    {
        auto result = m_app->update();

        if (result.isErr())
        {
            return pieces::ErrRef<core::Platform, std::string>(std::move(result.error()));
        }
    }

    return pieces::OkRef<core::Platform, std::string>(*this);
}

void AGDKPlatform::pause() { m_app->pause(); }

void AGDKPlatform::resume()
{
    auto context = static_cast<AGDKPlatformContext*>(getPlatformContext());

    if (context && context->getCurrentWindow())
    {
        m_app->resume();
    }
    else
    {
        throw std::runtime_error("AGDKPlatform: Cannot resume - no valid window");
    }
}

void AGDKPlatform::shutdown() { m_app->shutdown(); }

std::optional<bool> AGDKPlatform::showQuestionDialog(const std::string& _title,
                                                     const std::string& _message,
                                                     bool _allowCancel) const
{
    auto* helper = JNIHelper::getInstance();

    JNIEnv* env = helper->getEnv();
    if (!env) return std::nullopt;

    jstring jTitle = helper->stringToJstring(_title);
    jstring jMessage = helper->stringToJstring(_message);
    jboolean jAllowCancel = static_cast<jboolean>(_allowCancel);

    // See EngineBridge.showQuestionDialog(String title, String message, boolean allowCancel)
    jobject jResult = helper->callStaticMethod<jobject>(
        "com/mosaic/engine_bridge/EngineBridge", "showQuestionDialog",
        "(Ljava/lang/String;Ljava/lang/String;Z)Ljava/lang/Boolean;", jTitle, jMessage,
        jAllowCancel);

    if (jTitle) env->DeleteLocalRef(jTitle);
    if (jMessage) env->DeleteLocalRef(jMessage);
    if (!jResult) return std::nullopt;

    jclass booleanClass = env->FindClass("java/lang/Boolean");
    jmethodID booleanValue = env->GetMethodID(booleanClass, "booleanValue", "()Z");

    return env->CallBooleanMethod(jResult, booleanValue);
}

void AGDKPlatform::showInfoDialog(const std::string& _title, const std::string& _message) const
{
    auto* helper = JNIHelper::getInstance();

    JNIEnv* env = helper->getEnv();
    if (!env) return;

    jstring jTitle = helper->stringToJstring(_title);
    jstring jMessage = helper->stringToJstring(_message);

    // See EngineBridge.showInfoDialog(String title, String message)
    helper->callStaticVoidMethod("com/mosaic/engine_bridge/EngineBridge", "showInfoDialog",
                                 "(Ljava/lang/String;Ljava/lang/String;)V", jTitle, jMessage);

    if (jTitle) env->DeleteLocalRef(jTitle);
    if (jMessage) env->DeleteLocalRef(jMessage);
}

void AGDKPlatform::showWarningDialog(const std::string& _title, const std::string& _message) const
{
    auto* helper = JNIHelper::getInstance();

    JNIEnv* env = helper->getEnv();
    if (!env) return;

    jstring jTitle = helper->stringToJstring(_title);
    jstring jMessage = helper->stringToJstring(_message);

    // See EngineBridge.showWarningDialog(String title, String message)
    helper->callStaticVoidMethod("com/mosaic/engine_bridge/EngineBridge", "showWarningDialog",
                                 "(Ljava/lang/String;Ljava/lang/String;)V", jTitle, jMessage);

    if (jTitle) env->DeleteLocalRef(jTitle);
    if (jMessage) env->DeleteLocalRef(jMessage);
}

void AGDKPlatform::showErrorDialog(const std::string& _title, const std::string& _message) const
{
    auto* helper = JNIHelper::getInstance();

    JNIEnv* env = helper->getEnv();
    if (!env) return;

    jstring jTitle = helper->stringToJstring(_title);
    jstring jMessage = helper->stringToJstring(_message);

    // See EngineBridge.showErrorDialog(String title, String message)
    helper->callStaticVoidMethod("com/mosaic/engine_bridge/EngineBridge", "showErrorDialog",
                                 "(Ljava/lang/String;Ljava/lang/String;)V", jTitle, jMessage);

    if (jTitle) env->DeleteLocalRef(jTitle);
    if (jMessage) env->DeleteLocalRef(jMessage);
}

} // namespace agdk
} // namespace platform
} // namespace mosaic
