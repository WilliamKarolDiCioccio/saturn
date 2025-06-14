#pragma once

#include "mosaic/input/raw_input_handler.hpp"
#include "mosaic/platform/AGDK/agdk_platform.hpp"

namespace mosaic
{
namespace platform
{
namespace agdk
{

class AGDKRawInputHandler : public input::RawInputHandler
{
   private:
    window::Window* m_window;

   public:
    AGDKRawInputHandler(const window::Window* window);
    ~AGDKRawInputHandler() override = default;

   public:
    pieces::RefResult<input::RawInputHandler, std::string> initialize() override;
    void shutdown() override;

    input::RawInputHandler::KeyboardKeyInputData getKeyboardKeyInput(
        input::KeyboardKey key) const override;
    input::RawInputHandler::MouseButtonInputData getMouseButtonInput(
        input::MouseButton button) const override;
    input::RawInputHandler::CursorPosInputData getCursorPosInput() override;

   private:
    void registerCallbacks();
};

} // namespace agdk
} // namespace platform
} // namespace mosaic
