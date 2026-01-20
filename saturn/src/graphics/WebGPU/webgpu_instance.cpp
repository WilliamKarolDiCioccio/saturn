#include "webgpu_instance.hpp"

namespace saturn
{
namespace graphics
{
namespace webgpu
{

WGPUInstance createInstance()
{
    WGPUInstanceDescriptor instanceDesc = {};
    instanceDesc.nextInChain = nullptr;

#ifdef WEBGPU_BACKEND_DAWN
    WGPUDawnTogglesDescriptor toggles;
    toggles.chain.next = nullptr;
    toggles.chain.sType = WGPUSType_DawnTogglesDescriptor;
    toggles.disabledToggleCount = 0;
    toggles.enabledToggleCount = 1;
    const char* toggleName = "enable_immediate_error_handling";
    toggles.enabledToggles = &toggleName;

    instanceDesc.nextInChain = &toggles.chain;
#endif

#ifdef __EMSCRIPTEN__
    WGPUInstance instance = wgpuCreateInstance(nullptr);
#else
    WGPUInstance instance = wgpuCreateInstance(&instanceDesc);
#endif

    if (!instance)
    {
        SATURN_ERROR("Could not create WebGPU instance!");
    }

    return instance;
}

} // namespace webgpu
} // namespace graphics
} // namespace saturn
