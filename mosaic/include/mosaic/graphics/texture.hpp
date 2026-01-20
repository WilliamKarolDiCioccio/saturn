#pragma once

#include <string>

namespace saturn
{
namespace graphics
{

enum class TextureFormat
{
    RGBA8,
    Depth24Stencil8
};

enum class TextureUsage
{
    Sampled,
    RenderTarget,
    Storage
};

struct TextureDescription
{
    uint32_t width;
    uint32_t height;
    uint32_t mipLevels = 1;
    TextureFormat format;
    TextureUsage usage;
    bool cpuReadable = false;
    std::string debugName;
};

} // namespace graphics
} // namespace saturn
