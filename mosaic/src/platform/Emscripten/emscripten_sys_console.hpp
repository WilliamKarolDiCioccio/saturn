#include "mosaic/core/sysconsole.hpp"

#include <emscripten.h>

namespace mosaic
{
namespace platform
{
namespace emscripten
{

class EmscriptenSystemConsole : public core::SystemConsole::SystemConsoleImpl
{
   public:
    EmscriptenSystemConsole() = default;
    ~EmscriptenSystemConsole() override = default;

   public:
    void redirect() const override;
    void restore() const override;

    void print(const std::string& _message) const override;
    void printTrace(const std::string& _message) const override;
    void printDebug(const std::string& _message) const override;
    void printInfo(const std::string& _message) const override;
    void printWarn(const std::string& _message) const override;
    void printError(const std::string& _message) const override;
    void printCritical(const std::string& _message) const override;
};

} // namespace emscripten
} // namespace platform
} // namespace mosaic
