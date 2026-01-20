#pragma once

#include <string>

#include "buffer.hpp"
#include "shader.hpp"
#include "texture.hpp"
#include "pipeline.hpp"

namespace saturn
{
namespace graphics
{

enum class DrawCallType
{
    NonIndexed,
    Indexed,
    Instanced,
    IndexedInstanced,
    Indirect
};

using ResourceHandle = uint32_t;

struct DrawCall
{
    DrawCallType type;

    ResourceHandle pipeline;
    std::vector<ResourceHandle> vertexBuffers;
    ResourceHandle indexBuffer;
    ResourceHandle indirectBuffer;

    uint32_t vertexCount;
    uint32_t indexCount;
    uint32_t instanceCount;

    uint32_t firstVertex;
    uint32_t firstIndex;
    uint32_t vertexOffset;
    uint32_t firstInstance;

    uint64_t sortKey;

    std::string debugName;

    DrawCall(const std::string& _debugName)
        : type(DrawCallType::NonIndexed),
          pipeline(0),
          indexBuffer(0),
          indirectBuffer(0),
          vertexCount(0),
          indexCount(0),
          instanceCount(1),
          firstVertex(0),
          firstIndex(0),
          vertexOffset(0),
          firstInstance(0),
          sortKey(0),
          debugName(_debugName) {};
};

} // namespace graphics
} // namespace saturn
