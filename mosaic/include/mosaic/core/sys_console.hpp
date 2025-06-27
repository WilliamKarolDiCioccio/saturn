#pragma once

#include <string>

#include "mosaic/defines.hpp"

namespace mosaic
{
namespace core
{

class SystemConsole
{
   public:
    class SystemConsoleImpl
    {
       public:
        SystemConsoleImpl() = default;
        virtual ~SystemConsoleImpl() = default;

       public:
        virtual void redirect() const = 0;
        virtual void restore() const = 0;

        virtual void print(const std::string& _message) const = 0;
        virtual void printTrace(const std::string& _message) const = 0;
        virtual void printDebug(const std::string& _message) const = 0;
        virtual void printInfo(const std::string& _message) const = 0;
        virtual void printWarn(const std::string& _message) const = 0;
        virtual void printError(const std::string& _message) const = 0;
        virtual void printCritical(const std::string& _message) const = 0;
    };

   private:
    MOSAIC_API static std::unique_ptr<SystemConsoleImpl> impl;

   public:
    SystemConsole(const SystemConsole&) = delete;
    SystemConsole& operator=(const SystemConsole&) = delete;
    SystemConsole(SystemConsole&&) = delete;
    SystemConsole& operator=(SystemConsole&&) = delete;

   public:
    MOSAIC_API static void redirect();
    MOSAIC_API static void restore();

    MOSAIC_API static void print(const std::string& _message);
    MOSAIC_API static void printTrace(const std::string& _message);
    MOSAIC_API static void printDebug(const std::string& _message);
    MOSAIC_API static void printInfo(const std::string& _message);
    MOSAIC_API static void printWarn(const std::string& _message);
    MOSAIC_API static void printError(const std::string& _message);
    MOSAIC_API static void printCritical(const std::string& _message);
};

} // namespace core
} // namespace mosaic
