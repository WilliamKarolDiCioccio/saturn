#include "saturn/platform/AGDK/agdk_platform.hpp"

namespace saturn
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
      m_surfaceDestroyed(false){};

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

            SATURN_DEBUG("AGDKPlatformContext: Surface destroyed");
        }
        else
        {
            m_surfaceDestroyed = false;

            SATURN_DEBUG("AGDKPlatformContext: Surface updated");
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

    auto result = getApplication()->initialize();

    if (result.isErr())
    {
        return pieces::ErrRef<core::Platform, std::string>(std::move(result.error()));
    }

    return pieces::OkRef<core::Platform, std::string>(*this);
}

pieces::RefResult<core::Platform, std::string> AGDKPlatform::run()
{
    auto context = static_cast<AGDKPlatformContext*>(getPlatformContext());

    auto app = getApplication();

    if (app->shouldExit())
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

    if (app->isResumed() && context && context->getCurrentWindow())
    {
        auto result = app->update();

        if (result.isErr())
        {
            return pieces::ErrRef<core::Platform, std::string>(std::move(result.error()));
        }
    }

    return pieces::OkRef<core::Platform, std::string>(*this);
}

void AGDKPlatform::pause() { getApplication()->pause(); }

void AGDKPlatform::resume()
{
    auto context = static_cast<AGDKPlatformContext*>(getPlatformContext());

    if (context && context->getCurrentWindow())
    {
        getApplication()->resume();
    }
    else
    {
        throw std::runtime_error("AGDKPlatform: Cannot resume - no valid window");
    }
}

void AGDKPlatform::shutdown() { getApplication()->shutdown(); }

} // namespace agdk
} // namespace platform
} // namespace saturn
