#include "webgpu_device.hpp"

namespace saturn
{
namespace graphics
{
namespace webgpu
{

WGPUAdapter requestAdapter(WGPUInstance _instance, WGPUSurface _surface)
{
    struct UserData
    {
        WGPUAdapter adapter = nullptr;
        bool requestEnded = false;
    };

    UserData userData;

    WGPURequestAdapterCallback onAdapterRequestEnded =
        [](WGPURequestAdapterStatus _status, WGPUAdapter _adapter, WGPUStringView _message,
           void* _userData1, [[maybe_unused]] void* _userData2)
    {
        UserData& userData = *reinterpret_cast<UserData*>(_userData1);

        if (_status == WGPURequestAdapterStatus_Success)
        {
            userData.adapter = _adapter;
        }
        else
        {
            SATURN_ERROR("Could not request WebGPU adapter: {}", _message.data);
        }

        userData.requestEnded = true;
    };

    WGPURequestAdapterCallbackInfo callbackInfo = {
        .mode = WGPUCallbackMode::WGPUCallbackMode_AllowSpontaneous,
        .callback = onAdapterRequestEnded,
        .userdata1 = (void*)&userData,
    };

    WGPURequestAdapterOptions adapterOpts = {};
    adapterOpts.nextInChain = nullptr;
    adapterOpts.compatibleSurface = _surface;

    wgpuInstanceRequestAdapter(_instance, &adapterOpts, callbackInfo);

#ifdef __EMSCRIPTEN__
    while (!userData.requestEnded)
    {
        emscripten_sleep(100);
    }
#endif

    assert(userData.requestEnded);
    assert(userData.adapter);

    return userData.adapter;
}

bool isAdapterSuitable(WGPUAdapter _adapter)
{
#ifndef __EMSCRIPTEN__
    WGPULimits supportedLimits;
    supportedLimits.nextInChain = nullptr;

#ifdef WEBGPU_BACKEND_DAWN
    if (wgpuAdapterGetLimits(_adapter, &supportedLimits) != WGPUStatus_Success)
#elif defined WEBGPU_BACKEND_WGPU
    if (wgpuAdapterGetLimits(_adapter, &supportedLimits) != WGPUStatus_Success)
#else
#error "Unknown WebGPU backend!"
#endif
    {
        SATURN_ERROR("Could not get WebGPU adapter limits!");
        return false;
    }
#endif

    WGPUSupportedFeatures supportedFeatures = {};

    wgpuAdapterGetFeatures(_adapter, &supportedFeatures);

    if (supportedFeatures.featureCount == 0)
    {
        SATURN_ERROR("Could not get WebGPU adapter features!");
        return false;
    }

    WGPUAdapterInfo infos = {};

    wgpuAdapterGetInfo(_adapter, &infos);

    SATURN_INFO("WebGPU adapter device: {}", infos.device.data);
    SATURN_INFO("WebGPU adapter vendor: {}", infos.vendor.data);
    SATURN_INFO("WebGPU adapter description: {}", infos.description.data);
    SATURN_INFO("WebGPU adapter type: {}", static_cast<int>(infos.adapterType));
    SATURN_INFO("WebGPU adapter backendType: {}", static_cast<int>(infos.backendType));

    return true;
}

WGPUDevice createDevice(WGPUAdapter _adapter)
{
    struct UserData
    {
        WGPUDevice device = nullptr;
        bool requestEnded = false;
    };

    UserData userData;

    WGPURequestDeviceCallback onDeviceRequestEnded = [](WGPURequestDeviceStatus _status,
                                                        WGPUDevice _device, WGPUStringView _message,
                                                        void* _userData1, void* _userData2)
    {
        UserData& userData1 = *reinterpret_cast<UserData*>(_userData1);

        if (_status == WGPURequestDeviceStatus_Success)
        {
            userData1.device = _device;
        }
        else
        {
            SATURN_ERROR("Could not request WebGPU device: {}", _message.data);
        }

        userData1.requestEnded = true;
    };

    WGPUDeviceLostCallback onDeviceLost = [](const WGPUDevice* _device,
                                             WGPUDeviceLostReason _reason, WGPUStringView _message,
                                             void* _userData1, void* _userData2)
    { SATURN_ERROR("WebGPU device lost: {}", _message.data); };

    WGPUDeviceLostCallbackInfo onDeviceLostCallbackInfo = {
        .callback = onDeviceLost,
        .userdata1 = nullptr,
        .userdata2 = nullptr,
    };

    WGPUUncapturedErrorCallback onDeviceError = [](const WGPUDevice* _device, WGPUErrorType _type,
                                                   WGPUStringView _message, void* _userData1,
                                                   void* _userData2)
    {
        switch (_type)
        {
            case WGPUErrorType_Validation:
                SATURN_ERROR("WebGPU validation error: {}", _message.data);
                break;
            case WGPUErrorType_OutOfMemory:
                SATURN_ERROR("WebGPU out of memory: {}", _message.data);
                break;
            case WGPUErrorType_Unknown:
                SATURN_ERROR("WebGPU unknown error: {}", _message.data);
                break;
            default:
                SATURN_ERROR("WebGPU error: {}", _message.data);
                break;
        }
    };

    WGPUUncapturedErrorCallbackInfo uncapturedErrorCallbackInfo = {
        .callback = onDeviceError,
        .userdata1 = nullptr,
        .userdata2 = nullptr,
    };

    WGPURequestDeviceCallbackInfo callbackInfo = {
        .mode = WGPUCallbackMode::WGPUCallbackMode_AllowSpontaneous,
        .callback = onDeviceRequestEnded,
        .userdata1 = (void*)&userData,
        .userdata2 = nullptr,
    };

    WGPUDeviceDescriptor deviceDesc = {};
    deviceDesc.nextInChain = nullptr;
    deviceDesc.deviceLostCallbackInfo = onDeviceLostCallbackInfo;
    deviceDesc.uncapturedErrorCallbackInfo = uncapturedErrorCallbackInfo;

    wgpuAdapterRequestDevice(_adapter, &deviceDesc, callbackInfo);

#ifdef __EMSCRIPTEN__
    while (!userData.requestEnded)
    {
        emscripten_sleep(100);
    }
#endif

    assert(userData.requestEnded);

    return userData.device;
}

} // namespace webgpu
} // namespace graphics
} // namespace saturn
