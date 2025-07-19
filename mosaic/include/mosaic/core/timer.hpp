#pragma once

#include <chrono>
#include <ctime>
#include <functional>
#include <thread>
#include <vector>
#include <mutex>
#include <iostream>
#include <string>
#include <random>

#include "mosaic/internal/defines.hpp"

namespace mosaic
{
namespace core
{

/**
 * @brief Represents a scheduled callback.
 *
 * The ScheduledCallback struct contains information about a callback including its trigger time,
 * its UUID, and whether it has been cancelled.
 */
struct ScheduledCallback
{
    size_t id;
    double triggerTime;
    bool cancelled;
    std::function<void()> callback;

    ScheduledCallback(size_t _id, double _triggerTime, const std::function<void()>& _callback)
        : id(_id), triggerTime(_triggerTime), cancelled(false), callback(_callback) {};
};

/**
 * @brief The `Timer` class provides both a static interface for getting the current time and an
 * object instance interface for scheduling callbacks to be executed after a certain delay.
 */
class MOSAIC_API Timer
{
   public:
    /**
     * @brief Get the current time in seconds since the epoch.
     *
     * @return The current time in seconds since the epoch.
     */
    static double getCurrentTime();

    /**
     * @brief Get the current date as a string. The format is "Y-m-d H:M:S".
     *
     * @return The current date as a string.
     */
    static std::string getCurrentDate();

    /**
     * @brief Get the current time in seconds since the epoch.
     *
     * @return The current time in seconds since the epoch.
     */
    static double getDeltaTime();

    /**
     * @brief Sleep for a specified duration. Wraps std::this_thread::sleep_for.
     *
     * @param _seconds The duration to sleep for.
     */
    static void sleepFor(std::chrono::duration<double> _seconds);

    /**
     * @brief Tick the timer. This function updates the last time to the current time
     * so that the delta time can be calculated correctly.
     */
    static void tick() { s_lastTime = getCurrentTime(); }

   public:
    Timer() : m_running(true) { m_thread = std::thread(&Timer::runScheduledCallbacks, this); }

    ~Timer()
    {
        m_running = false;
        if (m_thread.joinable()) m_thread.join();
    }

   public:
    /**
     * @brief Schedule a callback to be executed after a certain delay.
     *
     * @param _delaySeconds The delay before the callback is executed.
     * @param _callback The callback function to be executed.
     *
     * @return A ScheduledCallback object representing the scheduled callback.
     */
    [[nodiscard]] size_t scheduleCallback(std::chrono::duration<double> _delaySeconds,
                                          std::function<void()> _callback);

    /**
     * @brief Cancel a scheduled callback.
     *
     * @param _uuid The UUID of the callback to cancel.
     */
    void cancelCallback(const size_t _id);

   private:
    /**
     * @brief Runs the scheduled callbacks in a separate thread.
     *
     * This function continuously checks for scheduled callbacks and executes them when their
     * trigger time is reached. It runs in a separate thread to avoid blocking the main application
     * thread.
     */
    void runScheduledCallbacks();

    static double s_lastTime;

    std::vector<ScheduledCallback> m_callbacks;
    std::mutex m_mutex;
    bool m_running;
    std::thread m_thread;
};

} // namespace core
} // namespace mosaic
