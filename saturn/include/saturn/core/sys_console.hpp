#pragma once

#include <string>

#include "saturn/defines.hpp"

namespace saturn
{
namespace core
{

/**
 * @brief Provides abstraction for system console, such as printing messages and managing console
 * state.
 */
class SystemConsole
{
   public:
    class SystemConsoleImpl
    {
       public:
        SystemConsoleImpl() = default;
        virtual ~SystemConsoleImpl() = default;

       public:
        virtual void attachParent() = 0;
        virtual void detachParent() = 0;
        virtual void create() const = 0;
        virtual void destroy() const = 0;

        virtual void print(const std::string& _message) const = 0;
        virtual void printTrace(const std::string& _message) const = 0;
        virtual void printDebug(const std::string& _message) const = 0;
        virtual void printInfo(const std::string& _message) const = 0;
        virtual void printWarn(const std::string& _message) const = 0;
        virtual void printError(const std::string& _message) const = 0;
        virtual void printCritical(const std::string& _message) const = 0;
    };

   private:
    SATURN_API static std::unique_ptr<SystemConsoleImpl> impl;

   public:
    SystemConsole(const SystemConsole&) = delete;
    SystemConsole& operator=(const SystemConsole&) = delete;
    SystemConsole(SystemConsole&&) = delete;
    SystemConsole& operator=(SystemConsole&&) = delete;

   public:
    SATURN_API static void attachParent();
    SATURN_API static void detachParent();
    SATURN_API static void create();
    SATURN_API static void destroy();

    SATURN_API static void print(const std::string& _message);
    SATURN_API static void printTrace(const std::string& _message);
    SATURN_API static void printDebug(const std::string& _message);
    SATURN_API static void printInfo(const std::string& _message);
    SATURN_API static void printWarn(const std::string& _message);
    SATURN_API static void printError(const std::string& _message);
    SATURN_API static void printCritical(const std::string& _message);
};

} // namespace core
} // namespace saturn
