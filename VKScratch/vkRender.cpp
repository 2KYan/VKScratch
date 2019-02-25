#include "vkRender.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#include <SDL2/SDL_vulkan.h>

template<class T, class Compare>
constexpr const T& clamp(const T& v, const T& lo, const T& hi, Compare comp)
{
    return assert(!comp(hi, lo)),
        comp(v, lo) ? lo : comp(hi, v) ? hi : v;
}

template<class T>
constexpr const T& clamp(const T& v, const T& lo, const T& hi)
{
    return clamp(v, lo, hi, std::less<>());
}

static std::vector<char const*> getDeviceExtensions()
{
    std::vector<char const*> extensions;

    extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

    return extensions;
}

static std::vector<char const*> getInstanceExtensions()
{
    std::vector<char const*> extensions;

    extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
#if  defined(VK_USE_PLATFORM_ANDROID_KHR)
    extensions.push_back(VK_KHR_ANDROID_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_IOS_MVK)
    extensions.push_back(VK_MVK_IOS_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_MACOS_MVK)
    extensions.push_back(VK_MVK_MACOS_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_MIR_KHR)
    extensions.push_back(VK_KHR_MIR_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_VI_NN)
    extensions.push_back(VK_NN_VI_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
    extensions.push_back(VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_WIN32_KHR)
    extensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_XCB_KHR)
    extensions.push_back(VK_KHR_XCB_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_XLIB_KHR)
    extensions.push_back(VK_KHR_XLIB_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_XLIB_XRANDR_EXT)
    extensions.push_back(VK_EXT_ACQUIRE_XLIB_DISPLAY_EXTENSION_NAME);
#endif

#if defined(_DEBUG)
    extensions.push_back("VK_EXT_debug_utils");
#endif

    return extensions;
}

static std::vector<char const*> getInstanceLayers()
{
    std::vector<char const*> layers;

#if defined(_DEBUG)
    layers.push_back("VK_LAYER_LUNARG_standard_validation");
#endif

    return layers;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {

    if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
    }

    return VK_FALSE;
}


vkRender::vkRender()
{
}


vkRender::~vkRender()
{
}

int vkRender::run()
{
    initWindow();
    initVulkan();
    mainLoop();
    cleanup();
}

int vkRender::initWindow()
{
    // Create an SDL window that supports Vulkan rendering.
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cout << "Could not initialize SDL." << std::endl;
        return 1;
    }
    SDL_Window* window = SDL_CreateWindow("Vulkan Window", SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED, m_width, m_height, SDL_WINDOW_VULKAN);
    if (window == NULL) {
        std::cout << "Could not create SDL window." << std::endl;
        return 1;
    }
    return 0;
}

int vkRender::initVulkan()
{
    createInstance();
    setupDebugMessenger();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapChain();
    createImageViews();


}

int vkRender::cleanup()
{

}

int vkRender::mainLoop()
{

}

void vkRender::createInstance()
{
    std::vector<vk::ExtensionProperties> availExtensions = vk::enumerateInstanceExtensionProperties();
    std::vector<vk::LayerProperties> availableLayers = vk::enumerateInstanceLayerProperties();

    std::vector<const char *> extensions = getInstanceExtensions();
    std::vector<const char *> layers = getInstanceLayers();

    vk::ApplicationInfo appInfo = vk::ApplicationInfo()
    .setPApplicationName("Vulkan C++ Program")
    .setApplicationVersion(1)
    .setPEngineName("2KYan_VK")
    .setEngineVersion(1)
    .setApiVersion(VK_API_VERSION_1_1);

    // vk::InstanceCreateInfo is where the programmer specifies the layers and/or extensions that
    // are needed.
    vk::InstanceCreateInfo instInfo = vk::InstanceCreateInfo()
    .setFlags(vk::InstanceCreateFlags())
    .setPApplicationInfo(&appInfo)
    .setEnabledExtensionCount(static_cast<uint32_t>(extensions.size()))
    .setPpEnabledExtensionNames(extensions.data())
    .setEnabledLayerCount(static_cast<uint32_t>(layers.size()))
    .setPpEnabledLayerNames(layers.data());
    
    m_vulkan.instance = vk::createInstanceUnique(instInfo);
    m_vulkan.dldi.init(*m_vulkan.instance);
}

void vkRender::setupDebugMessenger()
{
}

void vkRender::createSurface()
{
}

void vkRender::pickPhysicalDevice()
{
}

void vkRender::createLogicalDevice()
{
}

void vkRender::createSwapChain()
{
}

void vkRender::createImageViews()
{
}

