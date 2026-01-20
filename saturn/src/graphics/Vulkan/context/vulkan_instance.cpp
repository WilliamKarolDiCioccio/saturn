

#include "vulkan_instance.hpp"

namespace saturn
{
namespace graphics
{
namespace vulkan
{

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    [[maybe_unused]] VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, [[maybe_unused]] void* pUserData)
{
    switch (messageSeverity)
    {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
            SATURN_TRACE("{}", pCallbackData->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
            SATURN_INFO("{}", pCallbackData->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            SATURN_WARN("{}", pCallbackData->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            SATURN_ERROR("{}", pCallbackData->pMessage);
            break;
        default:
            break;
    }

    return VK_FALSE;
}

static VkResult createDebugUtilsMessengerEXT(VkInstance instance,
                                             const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
                                             const VkAllocationCallbacks* pAllocator,
                                             VkDebugUtilsMessengerEXT* pDebugMessenger)
{
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
        instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr)
    {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }
    else
    {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

static void destroyDebugUtilsMessengerEXT(VkInstance instance,
                                          VkDebugUtilsMessengerEXT debugMessenger,
                                          const VkAllocationCallbacks* pAllocator)
{
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
        instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr)
    {
        func(instance, debugMessenger, pAllocator);
    }
}

static constexpr VkDebugUtilsMessengerCreateInfoEXT populateDebugMessengerCreateInfoEXT()
{
    return {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .pNext = nullptr,
        .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
        .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        .pfnUserCallback = debugCallback,
        .pUserData = nullptr,
    };
}

void createDebugMessenger(Instance& _instance)
{
    auto createInfo = populateDebugMessengerCreateInfoEXT();

    if (createDebugUtilsMessengerEXT(_instance.instance, &createInfo, nullptr,
                                     &_instance.debugMessenger) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create Vulkan debug messenger!");
    }
}

void destroyDebugMessenger(Instance& _instance)
{
    destroyDebugUtilsMessengerEXT(_instance.instance, _instance.debugMessenger, nullptr);
}

void checkInstanceExtensionsSupport(Instance& _instance)
{
    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> extensions(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

    for (const auto& requiredExtension : _instance.requiredExtensions)
    {
        bool found = std::find_if(extensions.begin(), extensions.end(),
                                  [&](const VkExtensionProperties& extension)
                                  {
                                      return strncmp(requiredExtension, extension.extensionName,
                                                     VK_MAX_EXTENSION_NAME_SIZE);
                                  }) != extensions.end();

        if (found)
        {
            _instance.availableExtensions.push_back(requiredExtension);
        }
        else
        {
            return;
        }
    }

    for (const auto& optionalExtension : _instance.optionalExtensions)
    {
        bool found = std::find_if(extensions.begin(), extensions.end(),
                                  [&](const VkExtensionProperties& extension)
                                  {
                                      return strncmp(optionalExtension, extension.extensionName,
                                                     VK_MAX_EXTENSION_NAME_SIZE);
                                  }) != extensions.end();

        if (found)
        {
            _instance.availableExtensions.push_back(optionalExtension);
        }
        else
        {
            SATURN_WARN("Optional Vulkan instance extension '{}' is not supported!",
                        optionalExtension);
        }
    }
}

void checkInstanceLayersSupport(Instance& _instance)
{
    uint32_t layerCount = 0;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> layers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, layers.data());

    for (const auto& requiredLayer : _instance.requiredLayers)
    {
        bool found = std::find_if(layers.begin(), layers.end(),
                                  [&](const VkLayerProperties& layer)
                                  {
                                      return strncmp(requiredLayer, layer.layerName,
                                                     VK_MAX_EXTENSION_NAME_SIZE) == 0;
                                  }) != layers.end();

        if (found)
        {
            _instance.availableLayers.push_back(requiredLayer);
        }
        else
        {
            return;
        }
    }
}

void createInstance(Instance& _instance)
{
    if (volkInitialize() != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to initialize Vulkan loader!");
    }

    if (!volkGetInstanceVersion())
    {
        throw std::runtime_error("Vulkan is not supported on this platform!");
    }

    constexpr VkApplicationInfo appInfo = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext = nullptr,
        .pApplicationName = "Rake",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "Rake",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = VK_API_VERSION_1_3,
    };

    checkInstanceExtensionsSupport(_instance);
    checkInstanceLayersSupport(_instance);

    void* debugCreateInfoPtr = nullptr;

#if defined(SATURN_DEBUG_BUILD) || defined(SATURN_DEV_BUILD)
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = populateDebugMessengerCreateInfoEXT();

    debugCreateInfoPtr = &debugCreateInfo;
#endif

    const VkInstanceCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = debugCreateInfoPtr,
        .flags = 0,
        .pApplicationInfo = &appInfo,
        .enabledLayerCount = static_cast<uint32_t>(_instance.availableLayers.size()),
        .ppEnabledLayerNames = _instance.availableLayers.data(),
        .enabledExtensionCount = static_cast<uint32_t>(_instance.availableExtensions.size()),
        .ppEnabledExtensionNames = _instance.availableExtensions.data(),
    };

    if (vkCreateInstance(&createInfo, nullptr, &_instance.instance) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create Vulkan instance!");
    }

    volkLoadInstance(_instance.instance);

#if defined(SATURN_DEBUG_BUILD) || defined(SATURN_DEV_BUILD)
    createDebugMessenger(_instance);
#endif
}

void destroyInstance(Instance& _instance)
{
#if defined(SATURN_DEBUG_BUILD) || defined(SATURN_DEV_BUILD)
    destroyDebugMessenger(_instance);
#endif

    vkDestroyInstance(_instance.instance, nullptr);
}

} // namespace vulkan
} // namespace graphics
} // namespace saturn
