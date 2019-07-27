// Enable the WSI extensions
#if defined(__ANDROID__)
#define VK_USE_PLATFORM_ANDROID_KHR
#elif defined(__linux__)
#define VK_USE_PLATFORM_XLIB_KHR
#elif defined(_WIN32)
#define VK_USE_PLATFORM_WIN32_KHR
#endif

// Tell SDL not to mess with main()
//#define SDL_MAIN_HANDLED

#include <chrono>
#include <iostream>

#include "Camera.h"
#include "vkRender.h"
#include "vku.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#include "shaderc/shaderc.hpp"

//#include <SDL2/SDL.h>
//#include <SDL2/SDL_syswm.h>
#include <SDL2/SDL_vulkan.h>

#include "spdlog/spdlog.h"

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

vkRender::vkRender(SDL_Window* window, std::shared_ptr<Camera> pTrackBall, uint32_t width, uint32_t height)
{
    m_vulkan.window = window;
    m_pCamera = pTrackBall;
    m_width = width;
    m_height = height;

    initVulkan(width, height);
}


vkRender::~vkRender()
{
    m_vulkan.instance->destroyDebugUtilsMessengerEXT(m_vulkan.dbgMessenger, nullptr, m_vulkan.dldi);
}

int vkRender::initVulkan(uint32_t width, uint32_t height)
{
    createInstance();
    setupDebugMessenger();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();

    createSwapChain(width, height);
    createRenderPass();
    createDescriptorSetLayout();
    createGraphicsPipeline();
    createCommandPool();

    createColorResources();
    createDepthResources();
    createFrameBuffers();

    loadModel();

    createTextureImage();
    createTextureSampler();
    createVertexBuffer();
    createIndexBuffer();
    
    createUniformBuffer();
    createDescriptorPool();
    createDescriptorSets();
    createCommandBuffers();

    createSyncObjects();

    return 0;
}

void vkRender::cleanupSwapChain()
{
    for (auto& swapChainBuffer : m_vulkan.swapChain.frameBuffers) {
        swapChainBuffer.reset();
        // m_vulkan.device->destroyFramebuffer(*swapChainBuffer);
    }
    for (size_t i = 0; i < m_vulkan.commandBuffers.size(); ++i) {
        m_vulkan.commandBuffers[i].reset();
        //m_vulkan.uniformBuffer[i].reset();
        //m_vulkan.uniformBufferMemory[i].reset();
        m_vulkan.descriptorSets[i].reset();
    }

    m_vulkan.pipeLine.reset();
    m_vulkan.pipelineLayout.reset();
    m_vulkan.renderPass.reset();

    for (auto& imageView : m_vulkan.swapChain.views) {
        imageView.reset();
    }
    m_vulkan.swapChain.views.clear();
    m_vulkan.swapChain.swapChainKHR.reset();
}

void vkRender::recreateSwapChain(uint32_t width, uint32_t height)
{
    m_vulkan.device->waitIdle();

    cleanupSwapChain();

    createSwapChain(width, height);
    createRenderPass();
    createGraphicsPipeline();
    createColorResources();
    createDepthResources();
    createFrameBuffers();

    //createUniformBuffer();
    createDescriptorPool();
    createDescriptorSets();
    createCommandBuffers();
}

int vkRender::resizeWindow(int32_t width, int32_t height)
{
    recreateSwapChain(width, height);

    return 0;
}

void vkRender::findQueueFamilies(bool presentSupport = true)
{

    std::vector<vk::QueueFamilyProperties> qFamilies = m_vulkan.physicalDevice.getQueueFamilyProperties();

    m_vulkan.gQueue.familyIndex = -1;
    m_vulkan.pQueue.familyIndex = -1;
    int pqIdx = -1;
    for (auto& qFamily : qFamilies) {
        uint32_t qIdx = static_cast<uint32_t>(&qFamily - &qFamilies[0]);
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
    m_vulkan.udbgMessenger = m_vulkan.instance->createDebugUtilsMessengerEXTUnique(debugCreateInfo, nullptr, m_vulkan.dldi);
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
            m_vulkan.sampleCount = getMaxUsableSampleCount();
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

    if (m_vulkan.pQueue.familyIndex != m_vulkan.gQueue.familyIndex) {
        deviceQueueCreateInfo.setQueueFamilyIndex(m_vulkan.pQueue.familyIndex);
        dqCreateInfoArray.push_back(deviceQueueCreateInfo);
    }

    vk::DeviceCreateInfo deviceCreateInfo = vk::DeviceCreateInfo(vk::DeviceCreateFlags(), static_cast<uint32_t>(dqCreateInfoArray.size()), dqCreateInfoArray.data());
    auto deviceExtensions = getDeviceExtensions();
    deviceCreateInfo.setEnabledExtensionCount(static_cast<uint32_t>(deviceExtensions.size()));
    deviceCreateInfo.setPpEnabledExtensionNames(deviceExtensions.data());
    m_vulkan.device = m_vulkan.physicalDevice.createDeviceUnique(deviceCreateInfo);

    m_vulkan.gQueue.queue = m_vulkan.device->getQueue(m_vulkan.gQueue.familyIndex, 0);
    m_vulkan.pQueue.queue = m_vulkan.device->getQueue(m_vulkan.pQueue.familyIndex, 0);
}

void vkRender::createSwapChain(uint32_t width, uint32_t height)
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
                                         width));
        swapchainExtent.height = std::max(swapchainSupportDetails.capabilities.minImageExtent.height,
                                          std::min(swapchainSupportDetails.capabilities.maxImageExtent.height,
                                          height));
    }

    vk::SurfaceTransformFlagBitsKHR preTransform = (swapchainSupportDetails.capabilities.supportedTransforms & vk::SurfaceTransformFlagBitsKHR::eIdentity) ? 
                                                    vk::SurfaceTransformFlagBitsKHR::eIdentity : swapchainSupportDetails.capabilities.currentTransform;

    vk::CompositeAlphaFlagBitsKHR compositeAlpha =
        (swapchainSupportDetails.capabilities.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::ePreMultiplied) ? vk::CompositeAlphaFlagBitsKHR::ePreMultiplied :
        (swapchainSupportDetails.capabilities.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::ePostMultiplied) ? vk::CompositeAlphaFlagBitsKHR::ePostMultiplied :
        (swapchainSupportDetails.capabilities.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::eInherit) ? vk::CompositeAlphaFlagBitsKHR::eInherit : vk::CompositeAlphaFlagBitsKHR::eOpaque;
    
    uint32_t imageCount = swapchainSupportDetails.capabilities.minImageCount;
    if (swapchainSupportDetails.capabilities.maxImageCount > 0 && imageCount > swapchainSupportDetails.capabilities.maxImageCount) {
        imageCount = swapchainSupportDetails.capabilities.maxImageCount;
    }

    vk::SwapchainCreateInfoKHR swapChainCreateInfo(vk::SwapchainCreateFlagsKHR(), m_vulkan.surfaceKHR, imageCount, surfaceFormatKHR.format, surfaceFormatKHR.colorSpace,
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
    m_vulkan.swapChain.images = m_vulkan.device->getSwapchainImagesKHR(*m_vulkan.swapChain.swapChainKHR);
    m_vulkan.swapChain.format = surfaceFormatKHR.format;
    m_vulkan.swapChain.extent = swapchainExtent;

    vk::ComponentMapping componentMapping(vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eG, vk::ComponentSwizzle::eB, vk::ComponentSwizzle::eA);
    for (auto image : m_vulkan.swapChain.images) {
        vk::ImageSubresourceRange subResourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);
        vk::ImageViewCreateInfo imageViewCreateInfo(vk::ImageViewCreateFlags(), image, vk::ImageViewType::e2D, m_vulkan.swapChain.format, componentMapping, subResourceRange);
        //m_vulkan.swapChain.views.emplace_back(m_vulkan.device->createImageViewUnique(imageViewCreateInfo));
        m_vulkan.swapChain.views.emplace_back(utilCreateImageView(image, m_vulkan.swapChain.format, vk::ImageAspectFlagBits::eColor, 1));
    }
}

void vkRender::createRenderPass()
{
    vk::AttachmentDescription depthAttachment;
    depthAttachment.setFormat(findDepthFormat())
        .setSamples(m_vulkan.sampleCount)
        .setLoadOp(vk::AttachmentLoadOp::eClear)
        .setStoreOp(vk::AttachmentStoreOp::eDontCare)
        .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
        .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
        .setInitialLayout(vk::ImageLayout::eUndefined)
        .setFinalLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);

    vk::AttachmentReference depthAttachmentRef;
    depthAttachmentRef.setAttachment(1).setLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);

    vk::AttachmentDescription colorAttachment = {};
    colorAttachment.setFormat(m_vulkan.swapChain.format)
        .setSamples(m_vulkan.sampleCount)
        .setLoadOp(vk::AttachmentLoadOp::eClear).setStoreOp(vk::AttachmentStoreOp::eStore)
        .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare).setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
        .setInitialLayout(vk::ImageLayout::eUndefined)
        .setFinalLayout(vk::ImageLayout::eColorAttachmentOptimal);

    vk::AttachmentReference colorAttachmentRef;
    colorAttachmentRef.setAttachment(0).setLayout(vk::ImageLayout::eColorAttachmentOptimal);

    vk::AttachmentDescription colorAttachmentResolve;
    colorAttachmentResolve.setFormat(m_vulkan.swapChain.format)
        .setSamples(vk::SampleCountFlagBits::e1)
        .setLoadOp(vk::AttachmentLoadOp::eDontCare)
        .setStoreOp(vk::AttachmentStoreOp::eStore)
        .setFinalLayout(vk::ImageLayout::ePresentSrcKHR);
    
    vk::AttachmentReference colorAttachmentResolveRef;
    colorAttachmentResolveRef.setAttachment(2).setLayout(vk::ImageLayout::eColorAttachmentOptimal);

    vk::SubpassDescription subpass;
    subpass.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
        .setColorAttachmentCount(1)
        .setPColorAttachments(&colorAttachmentRef)
        .setPDepthStencilAttachment(&depthAttachmentRef)
        .setPResolveAttachments(&colorAttachmentResolveRef);

    vk::SubpassDependency dependency;
    dependency.setSrcSubpass(VK_SUBPASS_EXTERNAL).setDstSubpass(0)
        .setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
        .setSrcAccessMask(vk::AccessFlagBits::eColorAttachmentRead)
        .setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
        .setDstAccessMask(vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite);

    std::array<vk::AttachmentDescription, 3> attachments = { colorAttachment, depthAttachment, colorAttachmentResolve };
    vk::RenderPassCreateInfo renderPassInfo;
    renderPassInfo.setAttachmentCount(static_cast<uint32_t>(attachments.size()))
        .setPAttachments(attachments.data())
        .setSubpassCount(1)
        .setPSubpasses(&subpass)
        .setDependencyCount(1)
        .setPDependencies(&dependency);

    m_vulkan.renderPass = m_vulkan.device->createRenderPassUnique(renderPassInfo);
}

void vkRender::createDescriptorSetLayout()
{

    vk::DescriptorSetLayoutBinding uboLaytoutBinding;
    uboLaytoutBinding.setBinding(0)
        .setDescriptorCount(1)
        .setDescriptorType(vk::DescriptorType::eUniformBuffer)
        .setStageFlags(vk::ShaderStageFlagBits::eVertex);

    vk::DescriptorSetLayoutBinding samplerLayoutBinding;
    samplerLayoutBinding.setBinding(1)
        .setDescriptorCount(1)
        .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
        .setStageFlags(vk::ShaderStageFlagBits::eFragment);

    std::array<vk::DescriptorSetLayoutBinding, 2> bindings = { uboLaytoutBinding, samplerLayoutBinding };

    vk::DescriptorSetLayoutCreateInfo layoutInfo;
    layoutInfo.setBindingCount(static_cast<uint32_t>(bindings.size()))
        .setPBindings(bindings.data());

    m_vulkan.descriptorsetLayout = m_vulkan.device->createDescriptorSetLayoutUnique(layoutInfo);
}

void vkRender::createGraphicsPipeline()
{
    size_t shaderSize;
    auto vertShaderCode = vku::instance()->glslCompile("simple.vert", shaderSize, shaderc_vertex_shader);
    auto vertShaderCreateInfo = vk::ShaderModuleCreateInfo{ vk::ShaderModuleCreateFlags(), shaderSize, vertShaderCode.data() };
    auto vertShaderModule = m_vulkan.device->createShaderModuleUnique(vertShaderCreateInfo);

    auto fragShaderCode = vku::instance()->glslCompile("simple.frag", shaderSize, shaderc_fragment_shader);
    auto fragShaderCreateInfo = vk::ShaderModuleCreateInfo{ {}, shaderSize, fragShaderCode.data() };
    auto fragShaderModule = m_vulkan.device->createShaderModuleUnique(fragShaderCreateInfo);

    vk::PipelineShaderStageCreateInfo vertShaderStageInfo(vk::PipelineShaderStageCreateFlags(), vk::ShaderStageFlagBits::eVertex,*vertShaderModule, "main");
    vk::PipelineShaderStageCreateInfo fragShaderStageInfo(vk::PipelineShaderStageCreateFlags(), vk::ShaderStageFlagBits::eFragment, *fragShaderModule, "main");

    vk::PipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

    vk::PipelineVertexInputStateCreateInfo vertexInputInfo;
    vertexInputInfo.setVertexBindingDescriptionCount(1).setPVertexBindingDescriptions(&Vertex::getBindingDescription());
    auto vtxAttrDesc = Vertex::getAttributeDescription();
    vertexInputInfo.setVertexAttributeDescriptionCount(static_cast<uint32_t>(vtxAttrDesc.size())).setPVertexAttributeDescriptions(vtxAttrDesc.data());
    vk::PipelineInputAssemblyStateCreateInfo inputAssembly(vk::PipelineInputAssemblyStateCreateFlags(), vk::PrimitiveTopology::eTriangleList);

    vk::Viewport viewport(0, 0, float(m_vulkan.swapChain.extent.width), float(m_vulkan.swapChain.extent.height), 0.0, 1.0);
    vk::Rect2D scissor(vk::Offset2D(0, 0), vk::Extent2D(m_vulkan.swapChain.extent));
    vk::PipelineViewportStateCreateInfo viewportState(vk::PipelineViewportStateCreateFlags(), 1, &viewport, 1, &scissor);

    vk::PipelineRasterizationStateCreateInfo rasterizer(vk::PipelineRasterizationStateCreateFlags(), 0, 0, vk::PolygonMode::eFill, vk::CullModeFlagBits::eBack, vk::FrontFace::eClockwise, 0, 0, 0, 0, 1.0);


    vk::PipelineMultisampleStateCreateInfo multisampling;
    multisampling.setRasterizationSamples(m_vulkan.sampleCount);

    vk::PipelineDepthStencilStateCreateInfo depthStencil;
    depthStencil.setStencilTestEnable(0)
        .setDepthTestEnable(1)
        .setDepthWriteEnable(1)
        .setDepthCompareOp(vk::CompareOp::eLessOrEqual)
        .setDepthBoundsTestEnable(0)
        .setMinDepthBounds(0.0f)
        .setMaxDepthBounds(1.0f);

    vk::PipelineColorBlendAttachmentState colorBlendAttachment;
    colorBlendAttachment.setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA);
    vk::PipelineColorBlendStateCreateInfo colorBlending(vk::PipelineColorBlendStateCreateFlags(), 0, vk::LogicOp::eCopy, 1, &colorBlendAttachment);
    
    vk::PipelineLayoutCreateInfo pipelineLayoutInfo;
    pipelineLayoutInfo.setSetLayoutCount(1).setPSetLayouts(&*m_vulkan.descriptorsetLayout);
    m_vulkan.pipelineLayout = m_vulkan.device->createPipelineLayoutUnique(pipelineLayoutInfo);

    vk::GraphicsPipelineCreateInfo pipelineCreateInfo;
    pipelineCreateInfo
        .setBasePipelineHandle(vk::Pipeline())
        .setStageCount(2)
        .setPStages(shaderStages)
        .setPVertexInputState(&vertexInputInfo)
        .setPInputAssemblyState(&inputAssembly)
        .setPViewportState(&viewportState)
        .setPRasterizationState(&rasterizer)
        .setPMultisampleState(&multisampling)
        .setPDepthStencilState(&depthStencil)
        .setPColorBlendState(&colorBlending)
        .setLayout(*m_vulkan.pipelineLayout)
        .setRenderPass(*m_vulkan.renderPass)
        .setSubpass(0);

    m_vulkan.pipeLine = m_vulkan.device->createGraphicsPipelineUnique(vk::PipelineCache(), pipelineCreateInfo);

}


void vkRender::createFrameBuffers()
{
    m_vulkan.swapChain.frameBuffers.resize(m_vulkan.swapChain.views.size());

    for (size_t i = 0; i < m_vulkan.swapChain.views.size(); ++i) {
        std::array<vk::ImageView, 3> attachments = {
            *m_vulkan.colorImageView,
            *m_vulkan.depthImageView,
            *m_vulkan.swapChain.views[i],
        };

        vk::FramebufferCreateInfo frameBufferInfo;
        frameBufferInfo.setRenderPass(*m_vulkan.renderPass)
            .setAttachmentCount(static_cast<uint32_t>(attachments.size()))
            .setPAttachments(attachments.data())
            .setWidth(m_vulkan.swapChain.extent.width)
            .setHeight(m_vulkan.swapChain.extent.height)
            .setLayers(1);

        m_vulkan.swapChain.frameBuffers[i] = m_vulkan.device->createFramebufferUnique(frameBufferInfo);
    }
}

void vkRender::createCommandPool()
{
    vk::CommandPoolCreateInfo poolInfo;
    poolInfo.setQueueFamilyIndex(m_vulkan.gQueue.familyIndex);
    m_vulkan.commandPool = m_vulkan.device->createCommandPoolUnique(poolInfo);

}

void vkRender::createDepthResources()
{
    auto depthFormat = findDepthFormat();

    utilCreateImage(m_vulkan.swapChain.extent.width, m_vulkan.swapChain.extent.height, 1, m_vulkan.sampleCount, depthFormat, vk::ImageTiling::eOptimal,
                    vk::ImageUsageFlagBits::eDepthStencilAttachment, vk::MemoryPropertyFlagBits::eDeviceLocal,
                    m_vulkan.depthImage, m_vulkan.depthImageMemory);
    m_vulkan.depthImageView = utilCreateImageView(*m_vulkan.depthImage, depthFormat, vk::ImageAspectFlagBits::eDepth, 1);
    
    transitionImageLayout(m_vulkan.depthImage, depthFormat, vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthStencilAttachmentOptimal, 1);
}

void vkRender::createColorResources()
{
    auto colorFormat = m_vulkan.swapChain.format;

    utilCreateImage(m_vulkan.swapChain.extent.width, m_vulkan.swapChain.extent.height, 1, m_vulkan.sampleCount, colorFormat, vk::ImageTiling::eOptimal,
                    vk::ImageUsageFlagBits::eTransientAttachment|vk::ImageUsageFlagBits::eColorAttachment, vk::MemoryPropertyFlagBits::eDeviceLocal,
                    m_vulkan.colorImage, m_vulkan.colorImageMemory);
    m_vulkan.colorImageView = utilCreateImageView(*m_vulkan.colorImage, colorFormat, vk::ImageAspectFlagBits::eColor, 1);
    
    transitionImageLayout(m_vulkan.colorImage, colorFormat, vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal, 1);

}

void vkRender::createTextureImage()
{
    std::string texName = "chalet.jpg";
    int texWidth, texHeight, texChannel;
    std::string fname = vku::instance()->getTextureFileName(texName.c_str());
    if (fname.empty()) {
        spdlog::critical("Cant find texture {}", texName);
    }
    stbi_uc* pixels = stbi_load(fname.c_str(), &texWidth, &texHeight, &texChannel, STBI_rgb_alpha);
    if (!pixels) {
        throw std::runtime_error("failed to load texture image");
    }
    m_vulkan.mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;
    vk::DeviceSize imageSize = texWidth * texHeight * 4;
    vk::UniqueBuffer stagingBuffer;
    vk::UniqueDeviceMemory stagingBufferMemory;

    utilCreateBuffer(imageSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                 stagingBuffer, stagingBufferMemory);
    auto data = m_vulkan.device->mapMemory(*stagingBufferMemory, 0, imageSize);
    memcpy(data, pixels, imageSize);
    m_vulkan.device->unmapMemory(*stagingBufferMemory);
    stbi_image_free(pixels);

    utilCreateImage(texWidth, texHeight, m_vulkan.mipLevels, vk::SampleCountFlagBits::e1, vk::Format::eR8G8B8A8Unorm, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eTransferDst| vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eSampled, vk::MemoryPropertyFlagBits::eDeviceLocal, m_vulkan.textureImage, m_vulkan.textureImageMemory);

    transitionImageLayout(m_vulkan.textureImage, vk::Format::eR8G8B8A8Unorm, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, m_vulkan.mipLevels);
    copyBufferToImage(stagingBuffer, m_vulkan.textureImage, texWidth, texHeight);
    //transitionImageLayout(m_vulkan.textureImage, vk::Format::eR8G8B8A8Unorm, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);
    generateMipmaps(*m_vulkan.textureImage, vk::Format::eR8G8B8A8Unorm, texWidth, texHeight, m_vulkan.mipLevels);

    m_vulkan.textureImageView = utilCreateImageView(*m_vulkan.textureImage, vk::Format::eR8G8B8A8Unorm, vk::ImageAspectFlagBits::eColor, m_vulkan.mipLevels);
}

void vkRender::createTextureSampler()
{
    vk::SamplerCreateInfo samplerInfo;
    samplerInfo.setMinFilter(vk::Filter::eLinear)
        .setMagFilter(vk::Filter::eLinear)
        .setAddressModeU(vk::SamplerAddressMode::eRepeat)
        .setAddressModeV(vk::SamplerAddressMode::eRepeat)
        .setAddressModeW(vk::SamplerAddressMode::eRepeat)
        .setAnisotropyEnable(0)
        .setMaxAnisotropy(16)
        .setBorderColor(vk::BorderColor::eIntOpaqueBlack)
        .setCompareEnable(0)
        .setCompareOp(vk::CompareOp::eAlways)
        .setMipmapMode(vk::SamplerMipmapMode::eLinear)
        .setMipLodBias(0.0f)
        .setMinLod(0.0f)
        .setMaxLod(static_cast<float>(m_vulkan.mipLevels));

    m_vulkan.textureSampler = m_vulkan.device->createSamplerUnique(samplerInfo);

}

void vkRender::loadModel()
{
    m_vulkan.vertices = {
        {{-0.3f, -0.3f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
        {{0.3f, -0.3f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
        {{0.3f, 0.3f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
        {{-0.3f, 0.3f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},

        {{-0.5f, -0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
        {{0.5f, -0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
        {{0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
        {{-0.5f, 0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}
    };

    m_vulkan.indices = {
        0, 2, 1, 0, 3, 2,
        4, 6, 5, 4, 7, 6
    }; 

    return; 
    m_vulkan.vertices.clear();
    m_vulkan.indices.clear();

    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    std::string model_path = vku::instance()->getModelFileName("chalet.obj");

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, model_path.c_str())) {
        throw std::runtime_error(warn + err);
    }

    std::unordered_map<Vertex, uint32_t> uniqueVertices = {};

    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            Vertex vertex = {};

            vertex.pos = {
                attrib.vertices[3 * index.vertex_index + 0],
                attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2],
            };

            vertex.texCoord = {
                attrib.texcoords[2 * index.texcoord_index + 0],
                1.0 - attrib.texcoords[2 * index.texcoord_index + 1],
            };

            vertex.color = { 1.0f, 1.0f, 1.0f };

            if (uniqueVertices.count(vertex) == 0) {
                uniqueVertices[vertex] = static_cast<uint32_t>(m_vulkan.vertices.size());
                m_vulkan.vertices.push_back(vertex);
            } 

            m_vulkan.indices.push_back(uniqueVertices[vertex]);

        }
    }

}

void vkRender::createVertexBuffer()
{
    vk::DeviceSize bufferSize = sizeof(Vertex) * m_vulkan.vertices.size();
    vk::UniqueBuffer stagingBuffer;
    vk::UniqueDeviceMemory stagingBufferMemory;

    utilCreateBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                 stagingBuffer, stagingBufferMemory);

    auto data = m_vulkan.device->mapMemory(*stagingBufferMemory, 0, bufferSize);
    memcpy(data, m_vulkan.vertices.data(), bufferSize);
    m_vulkan.device->unmapMemory(*stagingBufferMemory);

    utilCreateBuffer(bufferSize, vk::BufferUsageFlagBits::eVertexBuffer|vk::BufferUsageFlagBits::eTransferDst, vk::MemoryPropertyFlagBits::eDeviceLocal,
                 m_vulkan.vertexBuffer, m_vulkan.vertexBufferMemory);

    copyBuffer(stagingBuffer, m_vulkan.vertexBuffer, bufferSize);
}

void vkRender::createIndexBuffer()
{
    vk::DeviceSize bufferSize = sizeof(uint32_t) * m_vulkan.indices.size();
    vk::UniqueBuffer stagingBuffer;
    vk::UniqueDeviceMemory stagingBufferMemory;
    utilCreateBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                 stagingBuffer, stagingBufferMemory);

    auto data = m_vulkan.device->mapMemory(*stagingBufferMemory, 0, bufferSize);
    memcpy(data, m_vulkan.indices.data(), bufferSize);
    m_vulkan.device->unmapMemory(*stagingBufferMemory);

    utilCreateBuffer(bufferSize, vk::BufferUsageFlagBits::eIndexBuffer|vk::BufferUsageFlagBits::eTransferDst, vk::MemoryPropertyFlagBits::eDeviceLocal,
                 m_vulkan.indexBuffer, m_vulkan.indexBufferMemory);

    copyBuffer(stagingBuffer, m_vulkan.indexBuffer, bufferSize);
}

void vkRender::createUniformBuffer()
{
    vk::DeviceSize bufferSize = sizeof(UniformBufferObject);
    size_t imageSize = m_vulkan.swapChain.images.size();
    m_vulkan.uniformBuffer.resize(imageSize);
    m_vulkan.uniformBufferMemory.resize(imageSize);

    for (size_t i = 0; i < imageSize; ++i) {
        utilCreateBuffer(bufferSize, vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                     m_vulkan.uniformBuffer[i], m_vulkan.uniformBufferMemory[i]);
    }
    
}

void vkRender::createDescriptorPool()
{
    uint32_t maxPoolSize = static_cast<uint32_t>(m_vulkan.swapChain.images.size());
    std::array<vk::DescriptorPoolSize, 2> poolSize;
    poolSize[0].setType(vk::DescriptorType::eUniformBuffer).setDescriptorCount(maxPoolSize);
    poolSize[1].setType(vk::DescriptorType::eCombinedImageSampler).setDescriptorCount(maxPoolSize);

    vk::DescriptorPoolCreateInfo poolInfo;
    poolInfo.setPoolSizeCount(static_cast<uint32_t>(poolSize.size()))
        .setPPoolSizes(poolSize.data()).setMaxSets(maxPoolSize).setFlags(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet);

    m_vulkan.descriptorPool = m_vulkan.device->createDescriptorPoolUnique(poolInfo);
}

void vkRender::createDescriptorSets()
{
    uint32_t maxSetSize = static_cast<uint32_t>(m_vulkan.swapChain.images.size());
    std::vector<vk::DescriptorSetLayout> layouts(m_vulkan.swapChain.images.size(), *m_vulkan.descriptorsetLayout);
    vk::DescriptorSetAllocateInfo allocInfo;
    allocInfo.setDescriptorPool(*m_vulkan.descriptorPool).setDescriptorSetCount(maxSetSize).setPSetLayouts(layouts.data());

    m_vulkan.descriptorSets = m_vulkan.device->allocateDescriptorSetsUnique(allocInfo);

    for (uint32_t i = 0; i < maxSetSize; ++i) {
        vk::DescriptorBufferInfo bufferInfo;
        bufferInfo.setBuffer(*m_vulkan.uniformBuffer[i]).setRange(sizeof(UniformBufferObject));
        vk::DescriptorImageInfo imageInfo;
        imageInfo.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal).setImageView(*m_vulkan.textureImageView).setSampler(*m_vulkan.textureSampler);

        std::array<vk::WriteDescriptorSet, 2> descriptorWrite;
        descriptorWrite[0].setDstSet(*m_vulkan.descriptorSets[i]).setDstBinding(0).setDescriptorType(vk::DescriptorType::eUniformBuffer).setDescriptorCount(1).setPBufferInfo(&bufferInfo);
        descriptorWrite[1].setDstSet(*m_vulkan.descriptorSets[i]).setDstBinding(1).setDescriptorType(vk::DescriptorType::eCombinedImageSampler).setDescriptorCount(1).setPImageInfo(&imageInfo);
        m_vulkan.device->updateDescriptorSets(descriptorWrite, nullptr);
    }
}

void vkRender::updateUniformBuffer(uint32_t index)
{
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
    
    if (m_vulkan.swapChain.extent.height == 0) {
        return;
    }
    
    UniformBufferObject ubo;
    ubo.modelview = m_pCamera->getModelView();
    ubo.proj = m_pCamera->getPerspective();
    //std::cout << glm::to_string(ubo.model) << std::endl;
    //ubo.model = glm::rotate(glm::mat4(1.0), time*glm::radians(10.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    //ubo.view = glm::lookAt(glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    //ubo.proj = glm::perspective(glm::radians(90.0f), 1.0f /*m_vulkan.swapChain.extent.width / float(m_vulkan.swapChain.extent.height)*/, 0.1f, 10.0f);
    //ubo.proj[1][1] = -1;

    auto data = m_vulkan.device->mapMemory(*m_vulkan.uniformBufferMemory[index], 0, sizeof(ubo));
    memcpy(data, &ubo, sizeof(ubo));
    m_vulkan.device->unmapMemory(*m_vulkan.uniformBufferMemory[index]);
}

void vkRender::createCommandBuffers() 
{
    vk::CommandBufferAllocateInfo allocInfo;
    allocInfo.setCommandBufferCount(static_cast<uint32_t>(m_vulkan.swapChain.views.size()))
        .setCommandPool(*m_vulkan.commandPool)
        .setLevel(vk::CommandBufferLevel::ePrimary);

    m_vulkan.commandBuffers = m_vulkan.device->allocateCommandBuffersUnique(allocInfo);

    for(size_t i=0;i<m_vulkan.commandBuffers.size();++i) {
        m_vulkan.commandBuffers[i]->begin(vk::CommandBufferBeginInfo().setFlags(vk::CommandBufferUsageFlagBits::eSimultaneousUse));
        vk::Rect2D renderArea(vk::Offset2D(0, 0), m_vulkan.swapChain.extent);
        std::array<vk::ClearValue, 2> clearValues;
        clearValues[0].color.float32[3] = 1.0;
        clearValues[1].depthStencil = { 1.0f, 0 };
        
        vk::RenderPassBeginInfo rpBeginInfo(*m_vulkan.renderPass, *m_vulkan.swapChain.frameBuffers[i],  renderArea, static_cast<uint32_t>(clearValues.size()), clearValues.data());
        m_vulkan.commandBuffers[i]->beginRenderPass(rpBeginInfo, vk::SubpassContents::eInline);
        m_vulkan.commandBuffers[i]->bindPipeline(vk::PipelineBindPoint::eGraphics, *m_vulkan.pipeLine);
        vk::DeviceSize offset =  0;
        m_vulkan.commandBuffers[i]->bindVertexBuffers(0, 1, &*m_vulkan.vertexBuffer, &offset);
        m_vulkan.commandBuffers[i]->bindIndexBuffer(*m_vulkan.indexBuffer, offset, vk::IndexType::eUint32);
        uint32_t dynamic_offset = 0;
        m_vulkan.commandBuffers[i]->bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *m_vulkan.pipelineLayout, 0, *m_vulkan.descriptorSets[i], nullptr);
        m_vulkan.commandBuffers[i]->drawIndexed(static_cast<uint32_t>(m_vulkan.indices.size()), 1, 0, 0, 0);
        m_vulkan.commandBuffers[i]->endRenderPass();

        m_vulkan.commandBuffers[i]->end();
    }

}

void vkRender::createSyncObjects()
{
    vk::SemaphoreCreateInfo semaphorInfo;
    m_vulkan.imageAvailableSemaphore.resize(m_max_frame_in_flight);
    m_vulkan.renderFinishedSemaphore.resize(m_max_frame_in_flight);
    m_vulkan.inFlightFences.resize(m_max_frame_in_flight);
    for (uint32_t i = 0; i < m_max_frame_in_flight; ++i) {
        m_vulkan.imageAvailableSemaphore[i] = m_vulkan.device->createSemaphoreUnique(semaphorInfo);
        m_vulkan.renderFinishedSemaphore[i] = m_vulkan.device->createSemaphoreUnique(semaphorInfo);
        m_vulkan.inFlightFences[i] = m_vulkan.device->createFenceUnique(vk::FenceCreateInfo().setFlags(vk::FenceCreateFlagBits::eSignaled));
    }

}

void vkRender::waitIdle()
{
    m_vulkan.device->waitIdle();
}

void vkRender::drawFrame()
{
    m_vulkan.device->waitForFences(1, &*m_vulkan.inFlightFences[m_currentFrame], VK_TRUE, std::numeric_limits<uint32_t>::max());

    try {
        auto imageIndex = m_vulkan.device->acquireNextImageKHR(*m_vulkan.swapChain.swapChainKHR, std::numeric_limits<uint32_t>::max(), *m_vulkan.imageAvailableSemaphore[m_currentFrame], vk::Fence());

        m_vulkan.device->resetFences(1, &*m_vulkan.inFlightFences[m_currentFrame]);

        updateUniformBuffer(imageIndex.value);

        vk::SubmitInfo submitInfo;
        const vk::PipelineStageFlags waitStages[] = { vk::PipelineStageFlagBits::eColorAttachmentOutput };
        submitInfo.setCommandBufferCount(1)
            .setPCommandBuffers(&*m_vulkan.commandBuffers[imageIndex.value])
            .setWaitSemaphoreCount(1)
            .setPWaitSemaphores(&*m_vulkan.imageAvailableSemaphore[m_currentFrame])
            .setPWaitDstStageMask(waitStages)
            .setSignalSemaphoreCount(1)
            .setPSignalSemaphores(&*m_vulkan.renderFinishedSemaphore[m_currentFrame]);
        m_vulkan.gQueue.queue.submit(1, &submitInfo, *m_vulkan.inFlightFences[m_currentFrame]);

        vk::PresentInfoKHR presentInfo;
        presentInfo.setWaitSemaphoreCount(1)
            .setPWaitSemaphores(&*m_vulkan.renderFinishedSemaphore[m_currentFrame])
            .setSwapchainCount(1)
            .setPSwapchains(&*m_vulkan.swapChain.swapChainKHR)
            .setPImageIndices(&imageIndex.value);
        m_vulkan.pQueue.queue.presentKHR(&presentInfo);

        m_currentFrame = (m_currentFrame + 1) % m_max_frame_in_flight;
    } catch (vk::OutOfDateKHRError  e) {
        recreateSwapChain(m_width, m_height);
        return;
    } catch (vk::SystemError e) {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

}

std::vector<vk::UniqueCommandBuffer> vkRender::beginSingleTimeCommands()
{
    vk::CommandBufferAllocateInfo allocInfo;
    allocInfo.setLevel(vk::CommandBufferLevel::ePrimary).setCommandBufferCount(1).setCommandPool(*m_vulkan.commandPool);
    auto commandBuffers = m_vulkan.device->allocateCommandBuffersUnique(allocInfo);
    commandBuffers[0]->begin(vk::CommandBufferBeginInfo().setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

    return commandBuffers;
}

void vkRender::endSingleTimeCommands(std::vector<vk::UniqueCommandBuffer>& commandBuffers)
{
    commandBuffers[0]->end();

    vk::SubmitInfo submitInfo;
    submitInfo.setCommandBufferCount(1).setPCommandBuffers(&*commandBuffers[0]);

    m_vulkan.gQueue.queue.submit(1, &submitInfo, nullptr);
    m_vulkan.gQueue.queue.waitIdle();
}

template <typename... Args>
void vkRender::submit(void (vkRender::*func)(Args...), Args... args)
{
    
}

void vkRender::transitionImageLayout(vk::UniqueImage& image, vk::Format format, vk::ImageLayout oldLayout, vk::ImageLayout newLayout, uint32_t mipLevels)
{
    auto commandBuffers = beginSingleTimeCommands();

    vk::ImageSubresourceRange range;
    range.setLayerCount(1).setLevelCount(mipLevels);

    vk::ImageAspectFlags aspectFlag;
    if (newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal) {
        aspectFlag |= vk::ImageAspectFlagBits::eDepth;
        if (hasStencilComponent(format)) {
            aspectFlag |= vk::ImageAspectFlagBits::eStencil;
        }
    } else {
        aspectFlag = vk::ImageAspectFlagBits::eColor;
    }
    range.setAspectMask(aspectFlag);

    vk::ImageMemoryBarrier barrier;
    barrier.setSubresourceRange(range).setOldLayout(oldLayout).setNewLayout(newLayout).setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED).setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
    barrier.setImage(*image);

    vk::PipelineStageFlags srcStage;
    vk::PipelineStageFlags dstStage;

    if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferDstOptimal) {
        barrier.setDstAccessMask(vk::AccessFlagBits::eTransferWrite);
        srcStage = vk::PipelineStageFlagBits::eTopOfPipe;
        dstStage = vk::PipelineStageFlagBits::eTransfer;
    } else if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eColorAttachmentOptimal) {
        barrier.setDstAccessMask(vk::AccessFlagBits::eColorAttachmentRead |vk::AccessFlagBits::eColorAttachmentWrite);
        srcStage = vk::PipelineStageFlagBits::eTopOfPipe;
        dstStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    } else if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal) {
        barrier.setDstAccessMask(vk::AccessFlagBits::eDepthStencilAttachmentRead|vk::AccessFlagBits::eDepthStencilAttachmentWrite);
        srcStage = vk::PipelineStageFlagBits::eTopOfPipe;
        dstStage = vk::PipelineStageFlagBits::eEarlyFragmentTests;
    } else if (oldLayout == vk::ImageLayout::eTransferDstOptimal && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal) {
        barrier.setDstAccessMask(vk::AccessFlagBits::eShaderRead).setSrcAccessMask(vk::AccessFlagBits::eTransferWrite);
        srcStage = vk::PipelineStageFlagBits::eTransfer;
        dstStage = vk::PipelineStageFlagBits::eFragmentShader;
    } else {
        throw std::invalid_argument("Unsupported layout transition");
    }

    vk::DependencyFlags flag;
    commandBuffers[0]->pipelineBarrier(srcStage, dstStage, flag, nullptr, nullptr, barrier);

    endSingleTimeCommands(commandBuffers);
}

void vkRender::generateMipmaps(vk::Image& image, vk::Format format, int32_t texWidth, int32_t texHeight, uint32_t mipLevels)
{
    // Check if image format supports linear blitting
    auto formatProperties = m_vulkan.physicalDevice.getFormatProperties(format);

    if (!(formatProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImageFilterLinear)) {
        throw std::runtime_error("texture image format does not support linear blitting!");
    }

    auto commandBuffers = beginSingleTimeCommands();

    vk::ImageSubresourceRange range;
    range.setAspectMask(vk::ImageAspectFlagBits::eColor).setLayerCount(1).setLevelCount(1);

    vk::ImageMemoryBarrier barrier = {};
    barrier.setImage(image).setSubresourceRange(range);

    int32_t mipWidth = texWidth;
    int32_t mipHeight = texHeight;
    vk::DependencyFlags flag;

    for (uint32_t i = 1; i < mipLevels; i++) {
        barrier.subresourceRange.setBaseMipLevel(i - 1);
        barrier.setOldLayout(vk::ImageLayout::eTransferDstOptimal)
            .setNewLayout(vk::ImageLayout::eTransferSrcOptimal)
            .setSrcAccessMask(vk::AccessFlagBits::eTransferWrite)
            .setDstAccessMask(vk::AccessFlagBits::eTransferRead);

        commandBuffers[0]->pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer, flag, nullptr, nullptr, barrier);

        vk::ImageBlit blit;
        blit.srcOffsets[0] = { 0, 0, 0 };
        blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
        blit.srcSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
        blit.srcSubresource.mipLevel = i - 1;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = 1;
        blit.dstOffsets[0] = { 0, 0, 0 };
        blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
        blit.dstSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
        blit.dstSubresource.mipLevel = i;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = 1;

        commandBuffers[0]->blitImage(image, vk::ImageLayout::eTransferSrcOptimal, image, vk::ImageLayout::eTransferDstOptimal, blit, vk::Filter::eLinear);

        barrier.setOldLayout(vk::ImageLayout::eTransferSrcOptimal)
            .setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
            .setSrcAccessMask(vk::AccessFlagBits::eTransferRead)
            .setDstAccessMask(vk::AccessFlagBits::eShaderRead);

        commandBuffers[0]->pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader, flag, nullptr, nullptr, barrier);

        if (mipWidth > 1) mipWidth /= 2;
        if (mipHeight > 1) mipHeight /= 2;
    }

    barrier.subresourceRange.setBaseMipLevel(mipLevels - 1);
    barrier.setOldLayout(vk::ImageLayout::eTransferDstOptimal)
        .setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
        .setSrcAccessMask(vk::AccessFlagBits::eTransferWrite)
        .setDstAccessMask(vk::AccessFlagBits::eShaderRead);

    commandBuffers[0]->pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader, flag, nullptr, nullptr, barrier);

    endSingleTimeCommands(commandBuffers);
}

void vkRender::copyBufferToImage(vk::UniqueBuffer& buffer, vk::UniqueImage& image, uint32_t width, uint32_t height)
{
    auto commandBuffers = beginSingleTimeCommands();

    vk::ImageSubresourceLayers subResourceLayers;
    subResourceLayers.setAspectMask(vk::ImageAspectFlagBits::eColor).setLayerCount(1);
    vk::BufferImageCopy region;
    region.setImageSubresource(subResourceLayers).setImageExtent(vk::Extent3D(width, height, 1));
    commandBuffers[0]->copyBufferToImage(*buffer, *image, vk::ImageLayout::eTransferDstOptimal, region);

    endSingleTimeCommands(commandBuffers);
}

void vkRender::copyBuffer(vk::UniqueBuffer& srcBuffer, vk::UniqueBuffer& dstBuffer, vk::DeviceSize size)
{
    auto commandBuffers = beginSingleTimeCommands();
    commandBuffers[0]->copyBuffer(*srcBuffer, *dstBuffer, vk::BufferCopy().setSize(size));
    endSingleTimeCommands(commandBuffers);
}

void vkRender::utilCreateImage(uint32_t width, uint32_t height, uint32_t mipLevels, vk::SampleCountFlagBits msaa, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties, vk::UniqueImage& image, vk::UniqueDeviceMemory& imageMemory)
{
    vk::ImageCreateInfo imageInfo;
    imageInfo.setImageType(vk::ImageType::e2D).setExtent(vk::Extent3D(width, height, 1)).setMipLevels(1).setArrayLayers(1)
        .setFormat(format).setTiling(tiling).setInitialLayout(vk::ImageLayout::eUndefined).setUsage(usage).setSharingMode(vk::SharingMode::eExclusive)
        .setSamples(msaa).setMipLevels(mipLevels);

    image = m_vulkan.device->createImageUnique(imageInfo);

    vk::MemoryRequirements memRequirements = m_vulkan.device->getImageMemoryRequirements(*image);
    vk::MemoryAllocateInfo allocInfo;
    allocInfo.setAllocationSize(memRequirements.size).setMemoryTypeIndex(findMemoryType(memRequirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal));
    imageMemory = m_vulkan.device->allocateMemoryUnique(allocInfo);
    m_vulkan.device->bindImageMemory(*image, *imageMemory, 0);
}

vk::UniqueImageView vkRender::utilCreateImageView(vk::Image& image, vk::Format format, vk::ImageAspectFlags aspectFlags, uint32_t mipLevels)
{
    vk::ImageViewCreateInfo viewInfo;
    viewInfo.setImage(image).setFormat(format).setViewType(vk::ImageViewType::e2D);
    vk::ImageSubresourceRange subResourceRange;
    subResourceRange.setAspectMask(aspectFlags).setLayerCount(1).setLevelCount(mipLevels);
    viewInfo.setSubresourceRange(subResourceRange);

    return m_vulkan.device->createImageViewUnique(viewInfo);
}

void vkRender::utilCreateBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties, vk::UniqueBuffer& buffer, vk::UniqueDeviceMemory& bufferMemory)
{
    vk::BufferCreateInfo bufferInfo;
    bufferInfo.setSize(size).setUsage(usage).setSharingMode(vk::SharingMode::eExclusive);
    buffer = m_vulkan.device->createBufferUnique(bufferInfo);

    vk::MemoryRequirements memRequirements = m_vulkan.device->getBufferMemoryRequirements(*buffer);
    vk::MemoryAllocateInfo allocInfo;
    allocInfo.setAllocationSize(memRequirements.size).setMemoryTypeIndex(findMemoryType(memRequirements.memoryTypeBits, properties));
    bufferMemory = m_vulkan.device->allocateMemoryUnique(allocInfo);
    m_vulkan.device->bindBufferMemory(*buffer, *bufferMemory, 0);
}

uint32_t vkRender::findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties)
{
    vk::PhysicalDeviceMemoryProperties memProperties = m_vulkan.physicalDevice.getMemoryProperties();

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    throw std::runtime_error("failed to find suitable memory type");
    

}

vk::Format vkRender::findDepthFormat()
{
    return findSupportedFormat({ vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint },
                               vk::ImageTiling::eOptimal,
                               vk::FormatFeatureFlagBits::eDepthStencilAttachment);
}

vk::Format vkRender::findSupportedFormat(const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features)
{
    for (auto format : candidates) {
        auto props = m_vulkan.physicalDevice.getFormatProperties(format);
        if (tiling == vk::ImageTiling::eLinear && (props.linearTilingFeatures & features) == features) {
            return format;
        } else if (tiling == vk::ImageTiling::eOptimal && (props.optimalTilingFeatures & features) == features) {
            return format;
        }

    }

    throw std::runtime_error("Failed to find supported format!");
}

vk::SampleCountFlagBits vkRender::getMaxUsableSampleCount()
{
    auto properties = m_vulkan.physicalDevice.getProperties();
    auto sampleCount = properties.limits.framebufferColorSampleCounts & properties.limits.framebufferDepthSampleCounts;
    if (sampleCount & vk::SampleCountFlagBits::e64) { return vk::SampleCountFlagBits::e64; }
    if (sampleCount & vk::SampleCountFlagBits::e32) { return vk::SampleCountFlagBits::e32; }
    if (sampleCount & vk::SampleCountFlagBits::e16) { return vk::SampleCountFlagBits::e16; }
    if (sampleCount & vk::SampleCountFlagBits::e8) { return vk::SampleCountFlagBits::e8; }
    if (sampleCount & vk::SampleCountFlagBits::e4) { return vk::SampleCountFlagBits::e4; }
    if (sampleCount & vk::SampleCountFlagBits::e2) { return vk::SampleCountFlagBits::e2; }

    return vk::SampleCountFlagBits::e1; 
}

bool vkRender::hasStencilComponent(vk::Format format)
{
    return format == vk::Format::eD32SfloatS8Uint || format == vk::Format::eD24UnormS8Uint;
}

