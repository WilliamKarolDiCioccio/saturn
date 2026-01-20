#pragma once

#include "saturn/defines.hpp"

#include <pieces/core/result.hpp>

#include "saturn/tools/logger.hpp"

namespace saturn
{
namespace tools
{

/**
 * @brief Default logging sink that outputs logs to the console.
 */
class SATURN_API DefaultSink final : public Sink
{
   public:
    ~DefaultSink() override = default;

    pieces::RefResult<Sink, std::string> initialize() override;
    void shutdown() override;

    void trace(const std::string& _message) const override;
    void debug(const std::string& _message) const override;
    void info(const std::string& _message) const override;
    void warn(const std::string& _message) const override;
    void error(const std::string& _message) const override;
    void critical(const std::string& _message) const override;
};

} // namespace tools
} // namespace saturn
