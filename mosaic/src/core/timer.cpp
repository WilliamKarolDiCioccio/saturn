#include "saturn/core/timer.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <ctime>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace saturn
{
namespace core
{

struct Timer::Impl
{
    static double s_lastTime;

    std::vector<ScheduledCallback> m_callbacks;
    std::mutex m_mutex;
    bool m_running;
    std::thread m_thread;

    Impl() : m_running(true) { m_thread = std::thread(&Impl::runScheduledCallbacks, this); }

    ~Impl()
    {
        m_running = false;
        if (m_thread.joinable()) m_thread.join();
    }

    [[nodiscard]] size_t scheduleCallback(std::chrono::duration<double> _delaySeconds,
                                          std::function<void()> _callback);

    void cancelCallback(const size_t _id);

    static double getCurrentTime();

    static std::string getCurrentDate();

    static double getDeltaTime();

    static void sleepFor(std::chrono::duration<double> _seconds);

    static void tick();

    /**
     * @brief Runs the scheduled callbacks in a separate thread.
     *
     * This function continuously checks for scheduled callbacks and executes them when their
     * trigger time is reached. It runs in a separate thread to avoid blocking the main application
     * thread.
     */
    void runScheduledCallbacks();
};

double Timer::Impl::s_lastTime = getCurrentTime();

Timer::Timer() : m_impl(new Impl()) {}

Timer::~Timer() { delete m_impl; }

double Timer::Impl::getCurrentTime()
{
    using namespace std::chrono;

    auto now = steady_clock::now();
    double seconds = duration_cast<duration<double>>(now.time_since_epoch()).count();
    return seconds;
}

std::string Timer::Impl::getCurrentDate()
{
#pragma warning(disable : 4996)
    std::time_t now = std::time(nullptr);
    char buf[64];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
    return std::string(buf);
#pragma warning(default : 4996)
}

double Timer::Impl::getDeltaTime()
{
    double now = getCurrentTime();
    double dt = now - s_lastTime;
    s_lastTime = now;
    return dt;
}

void Timer::Impl::sleepFor(std::chrono::duration<double> _seconds)
{
    std::this_thread::sleep_for(_seconds);
}

size_t Timer::Impl::scheduleCallback(std::chrono::duration<double> _delaySeconds,
                                     std::function<void()> _callback)
{
    static std::atomic<size_t> callbackIdCounter(0);

    callbackIdCounter++;

    double triggerTime = getCurrentTime() + _delaySeconds.count();

    std::lock_guard<std::mutex> lock(m_mutex);

    m_callbacks.emplace_back(callbackIdCounter, triggerTime, _callback);

    return callbackIdCounter.load();
}

void Timer::Impl::cancelCallback(const size_t _id)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = std::find_if(m_callbacks.begin(), m_callbacks.end(),
                           [_id](const ScheduledCallback& cb) { return cb.id == _id; });

    if (it != m_callbacks.end()) it->cancelled = true;
}

void Timer::Impl::tick() { s_lastTime = getCurrentTime(); }

void Timer::Impl::runScheduledCallbacks()
{
    using namespace std::chrono_literals;

    while (m_running)
    {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            double now = getCurrentTime();

            std::vector<size_t> callbacksToRemove;

            for (size_t i = 0; i < m_callbacks.size(); ++i)
            {
                if (m_callbacks[i].cancelled)
                {
                    callbacksToRemove.push_back(i);
                }
                else if (now >= m_callbacks[i].triggerTime)
                {
                    m_callbacks[i].callback();
                    callbacksToRemove.push_back(i);
                }
            }

            std::sort(callbacksToRemove.begin(), callbacksToRemove.end(), std::greater<size_t>());

            for (auto idx : callbacksToRemove) m_callbacks.erase(m_callbacks.begin() + idx);
        }

        sleepFor(0.1ms);
    }
}

size_t Timer::scheduleCallback(std::chrono::duration<double> _delaySeconds,
                               std::function<void()> _callback)
{
    return m_impl->scheduleCallback(_delaySeconds, _callback);
}

void Timer::cancelCallback(const size_t _id) { return m_impl->cancelCallback(_id); }

double Timer::getCurrentTime() { return Impl::getCurrentTime(); }

std::string Timer::getCurrentDate() { return Impl::getCurrentDate(); }

double Timer::getDeltaTime() { return Impl::getDeltaTime(); }

void Timer::sleepFor(std::chrono::duration<double> _seconds) { Impl::sleepFor(_seconds); }

void Timer::tick() { Impl::tick(); }

} // namespace core
} // namespace saturn
