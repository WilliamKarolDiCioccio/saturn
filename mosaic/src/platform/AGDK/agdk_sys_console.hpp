#include "saturn/core/sys_console.hpp"

#include <android/log.h>

namespace saturn
{
namespace platform
{
namespace agdk
{

class AGDKSystemConsole : public core::SystemConsole::SystemConsoleImpl
{
   public:
    AGDKSystemConsole() = default;
    ~AGDKSystemConsole() override = default;

   public:
    void attachParent() override;
    void detachParent() override;
    void create() const override;
    void destroy() const override;

    void print(const std::string& _message) const override;
    void printTrace(const std::string& _message) const override;
    void printDebug(const std::string& _message) const override;
    void printInfo(const std::string& _message) const override;
    void printWarn(const std::string& _message) const override;
    void printError(const std::string& _message) const override;
    void printCritical(const std::string& _message) const override;
};

} // namespace agdk
} // namespace platform
} // namespace saturn
