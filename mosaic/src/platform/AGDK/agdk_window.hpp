#pragma once

#include "mosaic/core/window.hpp"
#include "mosaic/platform/AGDK/agdk_platform.hpp"

namespace mosaic
{
namespace platform
{
namespace agdk
{

class AGDKWindow : public core::Window
{
   public:
    AGDKWindow(const std::string& _title, glm::ivec2 _size);
    ~AGDKWindow() override = default;

    void* getNativeHandle() const override;
    bool shouldClose() const override;
    glm::ivec2 getFramebufferSize() const override;

    void setTitle(const std::string& _title) override;
    void setMinimized(bool _minimized) override;
    void setMaximized(bool _maximized) override;
    void setFullscreen(bool _fullscreen) override;
    void setSize(glm::vec2 _size) override;
    void setResizeable(bool _resizeable) override;
    void setVSync(bool _enabled) override;

    void setCursorMode(core::CursorMode _mode) override;
    void setCursorType(core::CursorType _type) override;
    void setCursorTypeIcon(core::CursorType _type, const std::string& _path, int _width = 0,
                           int _height = 0) override;

    void setCursorIcon(const std::string& _path, int _width = 0, int _height = 0) override;
    void resetCursorIcon() override;
    void setWindowIcon(const std::string& _path, int _width = 0, int _height = 0) override;
    void resetWindowIcon() override;

    void setClipboardString(const std::string& _string) override;
    std::string getClipboardString() const override;
};

} // namespace agdk
} // namespace platform
} // namespace mosaic
