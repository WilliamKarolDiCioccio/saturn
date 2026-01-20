#include "vulkan_shader_module.hpp"

#include <vector>
#include <fstream>

#if defined(SATURN_PLATFORM_ANDROID)
#include <android/asset_manager.h>
#include <saturn/core/platform.hpp>
#include <saturn/platform/AGDK/agdk_platform.hpp>
#endif

namespace saturn
{
namespace graphics
{
namespace vulkan
{

static std::vector<char> readFile(const std::string& _filename)
{
#if defined(SATURN_PLATFORM_WINDOWS) || defined(SATURN_PLATFORM_LINUX) || \
    defined(SATURN_PLATFORM_MACOS)

    std::ifstream file(_filename, std::ios::ate | std::ios::binary);
    if (!file.is_open()) throw std::runtime_error("Failed to open shader file: " + _filename);

    size_t fileSize = static_cast<size_t>(file.tellg());
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();

    return buffer;

#elif defined(SATURN_PLATFORM_ANDROID)

    auto platform = saturn::core::Platform::getInstance();
    auto platformContext =
        static_cast<platform::agdk::AGDKPlatformContext*>(platform->getPlatformContext());

    AAsset* asset = AAssetManager_open(platformContext->getAssetManager(), _filename.c_str(),
                                       AASSET_MODE_BUFFER);

    if (!asset) throw std::runtime_error("Failed to open shader file: " + _filename);

    size_t fileSize = AAsset_getLength(asset);

    std::vector<char> buffer(fileSize);

    AAsset_read(asset, buffer.data(), fileSize);
    AAsset_close(asset);

    return buffer;

#endif
}

void createShaderModule(ShaderModule& _shaderModule, const VkDevice _device,
                        const std::string& _filepath)
{
    std::vector<char> code = readFile(_filepath);

    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    _shaderModule.device = _device;

    if (vkCreateShaderModule(_device, &createInfo, nullptr, &_shaderModule.shaderModule) !=
        VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create shader module from: " + _filepath);
    }
}

void destroyShaderModule(ShaderModule& _shaderModule)
{
    if (_shaderModule.shaderModule != VK_NULL_HANDLE)
    {
        vkDestroyShaderModule(_shaderModule.device, _shaderModule.shaderModule, nullptr);
        _shaderModule.shaderModule = VK_NULL_HANDLE;
    }
}

} // namespace vulkan
} // namespace graphics
} // namespace saturn
