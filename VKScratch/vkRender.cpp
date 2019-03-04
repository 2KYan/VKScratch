#include "vkRender.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#include <SDL2/SDL_vulkan.h>

#undef max
#undef min

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

    return 0;
}

int vkRender::initWindow()
{
    // Create an SDL window that supports Vulkan rendering.
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cout << "Could not initialize SDL." << std::endl;
        return 1;
    }
    m_vulkan.window = SDL_CreateWindow("Vulkan Window", SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED, m_width, m_height, SDL_WINDOW_VULKAN);
    if (m_vulkan.window == NULL) {
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

    return 0;
}

int vkRender::cleanup()
{
    return 0;
}

void vkRender::findQueueFamilies(bool presentSupport = true)
{

    std::vector<vk::QueueFamilyProperties> qFamilies = m_vulkan.physicalDevice.getQueueFamilyProperties();

    m_vulkan.gQueue.familyIndex = -1;
    m_vulkan.pQueue.familyIndex = -1;
    int pqIdx = -1;
    for (auto& qFamily : qFamilies) {
        uint32_t qIdx = &qFamily - &qFamilies[0];
        if (presentSupport && m_vulkan.physicalDevice.getSurfaceSupportKHR(qIdx, m_vulkan.surfaceKHR)) {
            m_vulkan.pQueue.familyIndex = qIdx;
        }
        if (qFamily.queueFlags & vk::QueueFlagBits::eGraphics) {
            m_vulkan.gQueue.familyIndex = qIdx;
        }

        if (m_vulkan.pQueue.familyIndex == m_vulkan.gQueue.familyIndex)  {
            break;
        }
    }

    return;
}

int vkRender::mainLoop()
{
    return 0;
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

    vk::DebugUtilsMessengerCreateInfoEXT debugCreateInfo = vk::DebugUtilsMessengerCreateInfoEXT()
        .setMessageSeverity(vk::DebugUtilsMessageSeverityFlagBitsEXT::eError | vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning)
        .setMessageType(vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance)
        .setPfnUserCallback(debugCallback)
        .setPUserData(nullptr);
    m_vulkan.dbgMessenger = m_vulkan.instance->createDebugUtilsMessengerEXT(debugCreateInfo, nullptr, m_vulkan.dldi);
}

void vkRender::createSurface()
{
    VkSurfaceKHR c_surface;
    if (!SDL_Vulkan_CreateSurface(m_vulkan.window, static_cast<VkInstance>(*m_vulkan.instance), &c_surface)) {
        std::cout << "Could not create a Vulkan surface." << std::endl;
        return;
    }
    m_vulkan.surfaceKHR = vk::SurfaceKHR(c_surface);
}

void vkRender::pickPhysicalDevice()
{
    std::vector<vk::PhysicalDevice> phyDevices = m_vulkan.instance->enumeratePhysicalDevices(m_vulkan.dldi);

    vk::PhysicalDevice physicalDevice;
    uint32_t pqIdx = -1;
    uint32_t gqIdx = -1;
    for (auto dev : phyDevices) {
        vk::PhysicalDeviceProperties deviceProperties = dev.getProperties();
        vk::PhysicalDeviceFeatures deviceFeatures = dev.getFeatures();
        //vk::PhysicalDeviceProperties2 deviceProperties2 = device.getProperties2(dldi);
        //vk::PhysicalDeviceFeatures2 deviceFeatures2 = device.getFeatures2(dldi);
        if (deviceProperties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu || deviceProperties.deviceType == vk::PhysicalDeviceType::eIntegratedGpu) {
            m_vulkan.physicalDevice = dev;
            return;
        } else {
            continue;
        }
    }
    //"Cant find right physicalDevice";
    return;
}

void vkRender::createLogicalDevice()
{
    findQueueFamilies();
    float queuePriority = 1.0f;
    std::vector<vk::DeviceQueueCreateInfo> dqCreateInfoArray;
    vk::DeviceQueueCreateInfo deviceQueueCreateInfo;
    deviceQueueCreateInfo.setPQueuePriorities(&queuePriority).setQueueCount(1).setQueueFamilyIndex(m_vulkan.gQueue.familyIndex);
    dqCreateInfoArray.push_back(deviceQueueCreateInfo);
    deviceQueueCreateInfo.setQueueFamilyIndex(m_vulkan.pQueue.familyIndex);
    dqCreateInfoArray.push_back(deviceQueueCreateInfo);

    vk::DeviceCreateInfo deviceCreateInfo = vk::DeviceCreateInfo(vk::DeviceCreateFlags(), dqCreateInfoArray.size(), dqCreateInfoArray.data());
    auto deviceExtensions = getDeviceExtensions();
    deviceCreateInfo.setEnabledExtensionCount(deviceExtensions.size());
    deviceCreateInfo.setPpEnabledExtensionNames(deviceExtensions.data());
    m_vulkan.device = m_vulkan.physicalDevice.createDeviceUnique(deviceCreateInfo);

    m_vulkan.gQueue.queue = m_vulkan.device->getQueue(m_vulkan.gQueue.familyIndex, 0);
    m_vulkan.pQueue.queue = m_vulkan.device->getQueue(m_vulkan.pQueue.familyIndex, 0);
}

void vkRender::createSwapChain()
{
    SwapChainSupportDetails swapchainSupportDetails;
    swapchainSupportDetails.capabilities = m_vulkan.physicalDevice.getSurfaceCapabilitiesKHR(m_vulkan.surfaceKHR);
    swapchainSupportDetails.presentModes = m_vulkan.physicalDevice.getSurfacePresentModesKHR(m_vulkan.surfaceKHR);
    swapchainSupportDetails.formats = m_vulkan.physicalDevice.getSurfaceFormatsKHR(m_vulkan.surfaceKHR);
    
    vk::SurfaceFormatKHR surfaceFormatKHR = { vk::Format::eB8G8R8A8Unorm, vk::ColorSpaceKHR::eSrgbNonlinear };
    if (swapchainSupportDetails.formats.size() != 1 || swapchainSupportDetails.formats[0].format != vk::Format::eUndefined) {
        surfaceFormatKHR = swapchainSupportDetails.formats[0];
        for (const auto& format : swapchainSupportDetails.formats) {
            if (format.format == vk::Format::eB8G8R8A8Unorm) {
                surfaceFormatKHR = format;
                break;
            }
        }

    }

    vk::PresentModeKHR swapchainPresentMode = vk::PresentModeKHR::eFifo;
    for (const auto& presentMode : swapchainSupportDetails.presentModes) {
        if (presentMode == vk::PresentModeKHR::eMailbox) {
            swapchainPresentMode = presentMode;
            break;
        } else if (presentMode == vk::PresentModeKHR::eImmediate) {
            swapchainPresentMode = presentMode;
        }
    }

    VkExtent2D swapchainExtent = swapchainSupportDetails.capabilities.currentExtent;
    if (swapchainSupportDetails.capabilities.currentExtent.width == std::numeric_limits<uint32_t>::max()) {
        swapchainExtent.width = std::max(swapchainSupportDetails.capabilities.minImageExtent.width,
                                         std::min(swapchainSupportDetails.capabilities.maxImageExtent.width,
                                         m_width));
        swapchainExtent.height = std::max(swapchainSupportDetails.capabilities.minImageExtent.height,
                                          std::min(swapchainSupportDetails.capabilities.maxImageExtent.height,
                                          m_height));
    }

    vk::SurfaceTransformFlagBitsKHR preTransform = (swapchainSupportDetails.capabilities.supportedTransforms & vk::SurfaceTransformFlagBitsKHR::eIdentity) ? 
                                                    vk::SurfaceTransformFlagBitsKHR::eIdentity : swapchainSupportDetails.capabilities.currentTransform;

    vk::CompositeAlphaFlagBitsKHR compositeAlpha =
        (swapchainSupportDetails.capabilities.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::ePreMultiplied) ? vk::CompositeAlphaFlagBitsKHR::ePreMultiplied :
        (swapchainSupportDetails.capabilities.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::ePostMultiplied) ? vk::CompositeAlphaFlagBitsKHR::ePostMultiplied :
        (swapchainSupportDetails.capabilities.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::eInherit) ? vk::CompositeAlphaFlagBitsKHR::eInherit : vk::CompositeAlphaFlagBitsKHR::eOpaque;

    vk::SwapchainCreateInfoKHR swapChainCreateInfo(vk::SwapchainCreateFlagsKHR(), m_vulkan.surfaceKHR, swapchainSupportDetails.capabilities.minImageCount, surfaceFormatKHR.format, surfaceFormatKHR.colorSpace,
                                                   swapchainExtent, 1, vk::ImageUsageFlagBits::eColorAttachment, vk::SharingMode::eExclusive, 0, nullptr, preTransform, compositeAlpha, swapchainPresentMode, true, nullptr);

    if (m_vulkan.gQueue.familyIndex !=m_vulkan.pQueue.familyIndex) {
        uint32_t queueFamilyIndices[2] = { m_vulkan.pQueue.familyIndex, m_vulkan.gQueue.familyIndex};
        // If the graphics and present queues are from different queue families, we either have to explicitly transfer ownership of images between
        // the queues, or we have to create the swapchain with imageSharingMode as VK_SHARING_MODE_CONCURRENT
        swapChainCreateInfo.imageSharingMode = vk::SharingMode::eConcurrent;
        swapChainCreateInfo.queueFamilyIndexCount = 2;
        swapChainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
    }

    m_vulkan.swapChain.swapChainKHR = m_vulkan.device->createSwapchainKHRUnique(swapChainCreateInfo);
    m_vulkan.swapChain.images = m_vulkan.device->getSwapchainImagesKHR(m_vulkan.swapChain.swapChainKHR.get());
    m_vulkan.swapChain.format = surfaceFormatKHR.format;
    m_vulkan.swapChain.extent = swapchainExtent;

}

void vkRender::createImageViews()
{
    std::vector<vk::UniqueImageView> imageViews;
    m_vulkan.swapChain.views.reserve(m_vulkan.swapChain.images.size());
    for (auto image : m_vulkan.swapChain.images) {
        vk::ComponentMapping componentMapping(vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eG, vk::ComponentSwizzle::eB, vk::ComponentSwizzle::eA);
        vk::ImageSubresourceRange subResourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);
        vk::ImageViewCreateInfo imageViewCreateInfo(vk::ImageViewCreateFlags(), image, vk::ImageViewType::e2D, m_vulkan.swapChain.format, componentMapping, subResourceRange);
        m_vulkan.swapChain.views.push_back(m_vulkan.device->createImageViewUnique(imageViewCreateInfo));
    }
}

