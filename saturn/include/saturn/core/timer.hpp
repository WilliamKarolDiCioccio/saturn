#pragma once

#include <chrono>
#include <functional>
#include <string>

#include "saturn/defines.hpp"

namespace saturn
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
class SATURN_API Timer
{
   private:
    struct Impl;

    Impl* m_impl;

   public:
    Timer();
    ~Timer();

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
    static void tick();
};

} // namespace core
} // namespace saturn
