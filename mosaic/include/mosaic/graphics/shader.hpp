#pragma once

#include <string>

namespace saturn
{
namespace graphics
{

enum class ShaderStage
{
    Vertex,
    Fragment,
    Compute
};

struct ShaderDescription
{
    ShaderStage stage;
    std::vector<uint8_t> bytecode;
    std::string entryPoint = "main";
    std::string debugName;
};

} // namespace graphics
} // namespace saturn
