#include "vulkan_device.hpp"
#include "vulkan_surface.hpp"

#include <vector>
#include <set>

namespace saturn
{
namespace graphics
{
namespace vulkan
{

std::string getDeviceTypeString(VkPhysicalDeviceType _type)
{
    switch (_type)
    {
        case VK_PHYSICAL_DEVICE_TYPE_OTHER:
            return "Other";
        case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
            return "Integrated GPU";
        case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
            return "Discrete GPU";
        case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
            return "Virtual GPU";
        case VK_PHYSICAL_DEVICE_TYPE_CPU:
            return "CPU";
        default:
            return "Unknown";
    }
}

bool isDeviceSuitable(const VkPhysicalDevice& _physicalDevice, const VkSurfaceKHR& _surface)
{
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(_physicalDevice, &deviceProperties);
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceFeatures(_physicalDevice, &deviceFeatures);

    QueueFamilySupportDetails queueFamiliySupport =
        findDeviceQueueFamiliesSupport(_physicalDevice, _surface);
    SwapChainSupportDetails swapChainSupport =
        findDeviceSwapChainSupport(_physicalDevice, _surface);

    return queueFamiliySupport.isComplete() && swapChainSupport.isComplete();
}

uint16_t getDeviceScore(const VkPhysicalDevice& _physicalDevice)
{
    VkPhysicalDeviceProperties deviceProperties;
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceProperties(_physicalDevice, &deviceProperties);
    vkGetPhysicalDeviceFeatures(_physicalDevice, &deviceFeatures);

    uint16_t score = 0;

    if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
    {
        score += 1000;
    }

    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceMemoryProperties(_physicalDevice, &memoryProperties);

    uint32_t heapCount = memoryProperties.memoryHeapCount;
    uint64_t totalDeviceMemory = 0;

    for (uint32_t j = 0; j < heapCount; ++j)
    {
        if (memoryProperties.memoryHeaps[j].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
        {
            totalDeviceMemory += memoryProperties.memoryHeaps[j].size;
        }
    }

    totalDeviceMemory /= 1024 * 1024;

    return score;
}

VkPhysicalDevice pickVulkanPhysicalDevice(const Instance& _instance, const VkSurfaceKHR& _surface)
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(_instance.instance, &deviceCount, nullptr);

    if (deviceCount == 0)
    {
        throw std::runtime_error("Failed to find GPUs with Vulkan support!");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(_instance.instance, &deviceCount, devices.data());

    uint32_t highestDeviceScore = 0;
    VkPhysicalDevice physicalDevice = nullptr;

    for (const auto& device : devices)
    {
        if (isDeviceSuitable(device, _surface))
        {
            VkPhysicalDeviceProperties deviceProperties;
            vkGetPhysicalDeviceProperties(device, &deviceProperties);

            uint16_t deviceScore = getDeviceScore(device);

            if (deviceScore >= highestDeviceScore)
            {
                highestDeviceScore = deviceScore;
                physicalDevice = device;
            }
        }
    }

    if (physicalDevice == nullptr)
    {
        SATURN_CRITICAL("No suitable Vulkan physical device found!");
        return nullptr;
    }

    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);

    SATURN_INFO("Vulkan pysical device name: '{}'", deviceProperties.deviceName);
    SATURN_INFO("Vulkan pysical device type: '{}'",
                getDeviceTypeString(deviceProperties.deviceType));
    SATURN_INFO("Vulkan pysical driver version: '{}'", deviceProperties.driverVersion);

    if (physicalDevice == nullptr)
    {
        throw std::runtime_error("Failed to find a suitable GPU!");
    }

    return physicalDevice;
}

QueueFamilySupportDetails findDeviceQueueFamiliesSupport(const VkPhysicalDevice& _device,
                                                         const VkSurfaceKHR& _surface)
{
    QueueFamilySupportDetails indices;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(_device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(_device, &queueFamilyCount, queueFamilies.data());

    int i = 0;
    for (const auto& queueFamily : queueFamilies)
    {
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            indices.graphicsFamily = i;
        }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(_device, i, _surface, &presentSupport);

        if (presentSupport)
        {
            indices.presentFamily = i;
        }

        if (indices.isComplete())
        {
            break;
        }

        i++;
    }

    return indices;
}

SwapChainSupportDetails findDeviceSwapChainSupport(const VkPhysicalDevice& device,
                                                   const VkSurfaceKHR& _surface)
{
    SwapChainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, _surface, &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, _surface, &formatCount, nullptr);

    if (formatCount != 0)
    {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, _surface, &formatCount,
                                             details.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, _surface, &presentModeCount, nullptr);

    if (presentModeCount != 0)
    {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, _surface, &presentModeCount,
                                                  details.presentModes.data());
    }

    return details;
}

void checkDeviceExtensionsSupport(Device& _device)
{
    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> extensions(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

    for (const auto& requiredExtension : _device.requiredExtensions)
    {
        bool found = std::find_if(extensions.begin(), extensions.end(),
                                  [&](const VkExtensionProperties& extension)
                                  {
                                      return strncmp(requiredExtension, extension.extensionName,
                                                     VK_MAX_EXTENSION_NAME_SIZE);
                                  }) != extensions.end();

        if (found)
        {
            _device.availableExtensions.push_back(requiredExtension);
        }
        else
        {
            return;
        }
    }

    for (const auto& optionalExtension : _device.optionalExtensions)
    {
        bool found = std::find_if(extensions.begin(), extensions.end(),
                                  [&](const VkExtensionProperties& extension)
                                  {
                                      return strncmp(optionalExtension, extension.extensionName,
                                                     VK_MAX_EXTENSION_NAME_SIZE);
                                  }) != extensions.end();

        if (found)
        {
            _device.availableExtensions.push_back(optionalExtension);
        }
        else
        {
            SATURN_WARN("Optional Vulkan device extension '{}' is not supported!",
                        optionalExtension);
        }
    }
}

void checkDeviceLayersSupport(Device& _device)
{
    uint32_t layerCount = 0;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> layers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, layers.data());

    for (const auto& requiredLayer : _device.requiredLayers)
    {
        bool found = std::find_if(layers.begin(), layers.end(),
                                  [&](const VkLayerProperties& layer)
                                  {
                                      return strncmp(requiredLayer, layer.layerName,
                                                     VK_MAX_EXTENSION_NAME_SIZE) == 0;
                                  }) != layers.end();

        if (found)
        {
            _device.availableLayers.push_back(requiredLayer);
        }
        else
        {
            return;
        }
    }
}

void createDevice(Device& _device, const Instance& _instance, const Surface& _surface)
{
    VkPhysicalDevice physicalDevice = pickVulkanPhysicalDevice(_instance, _surface.surface);

    checkDeviceExtensionsSupport(_device);
    checkDeviceLayersSupport(_device);

    _device.physicalDevice = physicalDevice;

    QueueFamilySupportDetails indices =
        findDeviceQueueFamiliesSupport(physicalDevice, _surface.surface);

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(),
                                              indices.presentFamily.value()};

    float queuePriority = 1.0f;

    for (uint32_t queueFamily : uniqueQueueFamilies)
    {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    constexpr VkPhysicalDeviceFeatures deviceFeatures{

    };

    const VkDeviceCreateInfo deviceCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = nullptr,
        .queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()),
        .pQueueCreateInfos = queueCreateInfos.data(),
        .enabledLayerCount = static_cast<uint32_t>(_device.availableLayers.size()),
        .ppEnabledLayerNames = _device.availableLayers.data(),
        .enabledExtensionCount = static_cast<uint32_t>(_device.availableExtensions.size()),
        .ppEnabledExtensionNames = _device.availableExtensions.data(),
        .pEnabledFeatures = &deviceFeatures,
    };

    auto res = vkCreateDevice(_device.physicalDevice, &deviceCreateInfo, nullptr, &_device.device);
    if (res != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create Vulkan device!");
    }

    volkLoadDevice(_device.device);

    vkGetDeviceQueue(_device.device, indices.graphicsFamily.value(), 0, &_device.graphicsQueue);
    vkGetDeviceQueue(_device.device, indices.presentFamily.value(), 0, &_device.presentQueue);
}

void destroyDevice(Device& _device) { vkDestroyDevice(_device.device, nullptr); }

} // namespace vulkan
} // namespace graphics
} // namespace saturn
