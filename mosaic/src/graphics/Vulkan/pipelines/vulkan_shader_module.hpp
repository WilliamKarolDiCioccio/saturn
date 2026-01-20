#pragma once

#include "../context/vulkan_device.hpp"

namespace saturn
{
namespace graphics
{
namespace vulkan
{

struct ShaderModule
{
    VkDevice device;
    VkShaderModule shaderModule;

    ShaderModule() : device(VK_NULL_HANDLE), shaderModule(VK_NULL_HANDLE) {}
};

void createShaderModule(ShaderModule& _shaderModule, const VkDevice _device,
                        const std::string& _filepath);

void destroyShaderModule(ShaderModule& _shaderModule);

} // namespace vulkan
} // namespace graphics
} // namespace saturn
