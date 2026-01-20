#pragma once

#include <string>

#include "shader.hpp"

namespace saturn
{
namespace graphics
{

enum class PrimitiveTopology
{
    TriangleList,
    TriangleStrip,
    LineList
};

enum class CullMode
{
    None,
    Front,
    Back
};

enum class FillMode
{
    Solid,
    Wireframe
};

enum class DepthCompareOp
{
    Less,
    LessEqual,
    Greater
};

struct RasterState
{
    CullMode cullMode = CullMode::Back;
    FillMode fillMode = FillMode::Solid;
    bool frontFaceCCW = true;
};

struct DepthState
{
    bool depthTestEnable = true;
    bool depthWriteEnable = true;
    DepthCompareOp compareOp = DepthCompareOp::LessEqual;
};

struct BlendState
{
    bool enable = false;
};

struct VertexInputAttribute
{
    std::string name;
    TextureFormat format;
    uint32_t location;
    uint32_t offset;
};

struct VertexLayout
{
    std::vector<VertexInputAttribute> attributes;
    uint32_t stride = 0;
};

struct PipelineDescription
{
    std::vector<ShaderDescription> shaders;
    VertexLayout vertexLayout;
    RasterState rasterState;
    DepthState depthState;
    BlendState blendState;
    PrimitiveTopology topology = PrimitiveTopology::TriangleList;
    std::string debugName;
};

} // namespace graphics
} // namespace saturn
