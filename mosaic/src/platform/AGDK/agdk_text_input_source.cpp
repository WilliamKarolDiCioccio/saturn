#include "agdk_text_input_source.hpp"

namespace saturn
{
namespace platform
{
namespace agdk
{

AGDKTextInputSource::AGDKTextInputSource(window::Window* window) : input::TextInputSource(window) {}

pieces::RefResult<input::InputSource, std::string> AGDKTextInputSource::initialize()
{
    return pieces::OkRef<input::InputSource, std::string>(*this);
};

void AGDKTextInputSource::shutdown() {};

void AGDKTextInputSource::pollDevice() {};

[[nodiscard]] inline std::vector<char32_t> AGDKTextInputSource::queryCodepoints() { return {}; }

} // namespace agdk
} // namespace platform
} // namespace saturn
