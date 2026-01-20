#pragma once

#include "saturn/input/sources/text_input_source.hpp"

namespace saturn
{
namespace platform
{
namespace agdk
{

class AGDKTextInputSource : public input::TextInputSource
{
   public:
    AGDKTextInputSource(window::Window* window);
    ~AGDKTextInputSource() override = default;

   public:
    pieces::RefResult<input::InputSource, std::string> initialize() override;
    void shutdown() override;
    void pollDevice() override;

   private:
    [[nodiscard]] std::vector<char32_t> queryCodepoints() override;
};

} // namespace agdk
} // namespace platform
} // namespace saturn
