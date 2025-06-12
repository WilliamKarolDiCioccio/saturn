#include "mosaic/core/logger.hpp"

#include <chrono>
#include <ctime>
#include <cassert>

#if defined(MOSAIC_PLATFORM_WINDOWS) || defined(MOSAIC_PLATFORM_LINUX) || \
    defined(MOSAIC_PLATFORM_MACOS)
#include <colorconsole.hpp>
#elif defined(MOSAIC_PLATFORM_EMSCRIPTEN)
#include <emscripten.h>
#elif defined(MOSAIC_PLATFORM_ANDROID)
#include <android/log.h>
#endif

namespace mosaic
{
namespace core
{

void DefaultSink::trace(const std::string& _message) const
{
#if defined(MOSAIC_PLATFORM_ANDROID)
    __android_log_print(ANDROID_LOG_DEBUG, "Mosaic", "%s", _message.c_str());
#elif defined(MOSAIC_PLATFORM_EMSCRIPTEN)
    emscripten_log(EM_LOG_DEBUG, __VA_ARGS__)
#else
    std::cout << dye::grey(_message) << '\n';
#endif
}

void DefaultSink::debug(const std::string& _message) const
{
#if defined(MOSAIC_PLATFORM_ANDROID)
    __android_log_print(ANDROID_LOG_DEBUG, "Mosaic", "%s", _message.c_str());
#elif defined(MOSAIC_PLATFORM_EMSCRIPTEN)
    emscripten_log(EM_LOG_DEBUG, __VA_ARGS__)
#else
    std::cout << dye::blue(_message) << '\n';
#endif
}

void DefaultSink::info(const std::string& _message) const
{
#if defined(MOSAIC_PLATFORM_ANDROID)
    __android_log_print(ANDROID_LOG_INFO, "Mosaic", "%s", _message.c_str());
#elif defined(MOSAIC_PLATFORM_EMSCRIPTEN)
    emscripten_log(EM_LOG_INFO, __VA_ARGS__)
#else
    std::cout << dye::green(_message) << '\n';
#endif
}

void DefaultSink::warn(const std::string& _message) const
{
#if defined(MOSAIC_PLATFORM_ANDROID)
    __android_log_print(ANDROID_LOG_WARN, "Mosaic", "%s", _message.c_str());
#elif defined(MOSAIC_PLATFORM_EMSCRIPTEN)
    emscripten_log(EM_LOG_WARN, __VA_ARGS__)
#else
    std::cout << dye::yellow(_message) << '\n';
#endif
}

void DefaultSink::error(const std::string& _message) const
{
#if defined(MOSAIC_PLATFORM_ANDROID)
    __android_log_print(ANDROID_LOG_ERROR, "Mosaic", "%s", _message.c_str());
#elif defined(MOSAIC_PLATFORM_EMSCRIPTEN)
    emscripten_log(EM_LOG_ERROR, __VA_ARGS__)
#else
    std::cout << dye::red(_message) << '\n';
#endif
}

void DefaultSink::critical(const std::string& _message) const
{
#if defined(MOSAIC_PLATFORM_ANDROID)
    __android_log_print(ANDROID_LOG_ERROR, "Mosaic", "%s", _message.c_str());
#elif defined(MOSAIC_PLATFORM_EMSCRIPTEN)
    emscripten_log(EM_LOG_ERROR, __VA_ARGS__)
#else
    std::cout << dye::purple(_message) << '\n';
#endif
}

LoggerManager* LoggerManager::s_instance = nullptr;

LoggerManager::LoggerManager(const LoggerConfig& _config) : m_config(_config)
{
    assert(!s_instance && "Logger instance already exists!");

    s_instance = this;
}

bool LoggerManager::initialize(const LoggerConfig& _config) noexcept
{
    if (s_instance) return false;

    s_instance = new LoggerManager(_config);

    return true;
}

void LoggerManager::shutdown() noexcept
{
    if (!s_instance) return;

    delete s_instance;
    s_instance = nullptr;
}

} // namespace core
} // namespace mosaic
