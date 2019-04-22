// Enable the WSI extensions
#if defined(__ANDROID__)
#define VK_USE_PLATFORM_ANDROID_KHR
#elif defined(__linux__)
#define VK_USE_PLATFORM_XLIB_KHR
#elif defined(_WIN32)
#define VK_USE_PLATFORM_WIN32_KHR
#endif

// Tell SDL not to mess with main()
#define SDL_MAIN_HANDLED

#include <chrono>

#include "vkRender.h"
#include "vku.h"

#include "shaderc/shaderc.hpp"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

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
    SDL_SetWindowResizable(m_vulkan.window, SDL_TRUE);
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
    createRenderPass();
    createDescriptorSetLayout();
    createGraphicsPipeline();
    createFrameBuffers();

    createCommandPool();
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
        m_vulkan.uniformBuffer[i].reset();
        m_vulkan.uniformBufferMemory[i].reset();
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

void vkRender::recreateSwapChain()
{
    m_vulkan.device->waitIdle();

    cleanupSwapChain();

    createSwapChain();
    createImageViews();
    createRenderPass();
    createGraphicsPipeline();
    createFrameBuffers();

    createUniformBuffer();
    createDescriptorPool();
    createDescriptorSets();
    createCommandBuffers();
}

int vkRender::cleanup()
{
    m_vulkan.instance->destroyDebugUtilsMessengerEXT(m_vulkan.dbgMessenger, nullptr, m_vulkan.dldi);

    SDL_DestroyWindow(m_vulkan.window);
    SDL_Quit();
    return 0;
}

int vkRender::resizeWindow(int32_t width, int32_t height)
{
    m_width = width;
    m_height = height;
    recreateSwapChain();

    return 0;
}

int vkRender::mainLoop()
{
    bool stillRunning = true;
    while (stillRunning) {

        SDL_Event event;
        while (SDL_PollEvent(&event)) {

            switch (event.type) {
            case SDL_WINDOWEVENT:
                if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
                    resizeWindow(event.window.data1, event.window.data2);
                }
                break;

            case SDL_QUIT:
                stillRunning = false;
                break;

            default:
                // Do nothing.
                break;
            }
        }
        drawFrame();

        SDL_Delay(10);
    }
    m_vulkan.device->waitIdle();

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
    m_vulkan.dldi.init(*m_vulkan.instance, vkGetInstanceProcAddr);
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

}

void vkRender::createImageViews()
{
    for (auto image : m_vulkan.swapChain.images) {
        vk::ComponentMapping componentMapping(vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eG, vk::ComponentSwizzle::eB, vk::ComponentSwizzle::eA);
        vk::ImageSubresourceRange subResourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);
        vk::ImageViewCreateInfo imageViewCreateInfo(vk::ImageViewCreateFlags(), image, vk::ImageViewType::e2D, m_vulkan.swapChain.format, componentMapping, subResourceRange);
        m_vulkan.swapChain.views.emplace_back(m_vulkan.device->createImageViewUnique(imageViewCreateInfo));
    }
}

void vkRender::createRenderPass()
{
    vk::AttachmentDescription colorAttachment = {};
    colorAttachment.setFormat(m_vulkan.swapChain.format).setSamples(vk::SampleCountFlagBits::e1);
    colorAttachment.setLoadOp(vk::AttachmentLoadOp::eClear).setStoreOp(vk::AttachmentStoreOp::eStore);
    colorAttachment.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare).setStencilStoreOp(vk::AttachmentStoreOp::eDontCare);
    colorAttachment.setInitialLayout(vk::ImageLayout::eUndefined).setFinalLayout(vk::ImageLayout::ePresentSrcKHR);

    vk::AttachmentReference colorAttachmentRef;
    colorAttachmentRef.setAttachment(0).setLayout(vk::ImageLayout::eColorAttachmentOptimal);
    
    vk::SubpassDescription subpass;
    subpass.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics).setColorAttachmentCount(1).setPColorAttachments(&colorAttachmentRef);

    vk::SubpassDependency dependency;
    dependency.setSrcSubpass(VK_SUBPASS_EXTERNAL).setDstSubpass(0)
        .setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
        .setSrcAccessMask(vk::AccessFlagBits::eColorAttachmentRead)
        .setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
        .setDstAccessMask(vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite);

    vk::RenderPassCreateInfo renderPassInfo;
    renderPassInfo.setAttachmentCount(1)
        .setPAttachments(&colorAttachment)
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

    vk::DescriptorSetLayoutCreateInfo layoutInfo;
    layoutInfo.setBindingCount(1).setPBindings(&uboLaytoutBinding);

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

    vk::PipelineRasterizationStateCreateInfo rasterizer(vk::PipelineRasterizationStateCreateFlags(), 0, 0, vk::PolygonMode::eFill, vk::CullModeFlags(), vk::FrontFace::eClockwise, 0, 0, 0, 0, 1.0);

    vk::PipelineMultisampleStateCreateInfo multisampling;

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
        vk::ImageView attachments[] = {
            *m_vulkan.swapChain.views[i]
        };

        vk::FramebufferCreateInfo frameBufferInfo;
        frameBufferInfo.setRenderPass(*m_vulkan.renderPass)
            .setAttachmentCount(1)
            .setPAttachments(attachments)
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

void vkRender::createVertexBuffer()
{
    m_vulkan.vertices = {
        {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
        {{ 0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
        {{ 0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}},
        {{-0.5f,  0.5f}, {1.0f, 1.1f, 1.0f}},
    };


    vk::DeviceSize bufferSize = sizeof(Vertex) * m_vulkan.vertices.size();
    vk::UniqueBuffer stagingBuffer;
    vk::UniqueDeviceMemory stagingBufferMemory;

    createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                 stagingBuffer, stagingBufferMemory);

    auto data = m_vulkan.device->mapMemory(*stagingBufferMemory, 0, bufferSize);
    memcpy(data, m_vulkan.vertices.data(), bufferSize);
    m_vulkan.device->unmapMemory(*stagingBufferMemory);

    createBuffer(bufferSize, vk::BufferUsageFlagBits::eVertexBuffer|vk::BufferUsageFlagBits::eTransferDst, vk::MemoryPropertyFlagBits::eDeviceLocal,
                 m_vulkan.vertexBuffer, m_vulkan.vertexBufferMemory);

    copyBuffer(stagingBuffer, m_vulkan.vertexBuffer, bufferSize);
}

void vkRender::createIndexBuffer()
{
    m_vulkan.indices = { 0 , 1, 2, 2, 3, 0 };

    vk::DeviceSize bufferSize = sizeof(uint16_t) * m_vulkan.indices.size();
    vk::UniqueBuffer stagingBuffer;
    vk::UniqueDeviceMemory stagingBufferMemory;
    createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                 stagingBuffer, stagingBufferMemory);

    auto data = m_vulkan.device->mapMemory(*stagingBufferMemory, 0, bufferSize);
    memcpy(data, m_vulkan.indices.data(), bufferSize);
    m_vulkan.device->unmapMemory(*stagingBufferMemory);

    createBuffer(bufferSize, vk::BufferUsageFlagBits::eIndexBuffer|vk::BufferUsageFlagBits::eTransferDst, vk::MemoryPropertyFlagBits::eDeviceLocal,
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
        createBuffer(bufferSize, vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                     m_vulkan.uniformBuffer[i], m_vulkan.uniformBufferMemory[i]);
    }
    
}

void vkRender::createDescriptorPool()
{
    uint32_t maxPoolSize = static_cast<uint32_t>(m_vulkan.swapChain.images.size());
    vk::DescriptorPoolSize poolSize;
    poolSize.setType(vk::DescriptorType::eUniformBuffer).setDescriptorCount(maxPoolSize);

    vk::DescriptorPoolCreateInfo poolInfo;
    poolInfo.setPoolSizeCount(1).setPPoolSizes(&poolSize).setMaxSets(maxPoolSize).setFlags(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet);

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
        vk::WriteDescriptorSet descriptorWrite;
        descriptorWrite.setDstSet(*m_vulkan.descriptorSets[i]).setDescriptorType(vk::DescriptorType::eUniformBuffer).setDescriptorCount(1).setPBufferInfo(&bufferInfo);
        m_vulkan.device->updateDescriptorSets(descriptorWrite, nullptr);
    }
}

void vkRender::updateUniformBuffer(uint32_t index)
{
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
    
    UniformBufferObject ubo;
    ubo.model = glm::rotate(glm::mat4(1.0), time*glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.proj = glm::perspective(glm::radians(45.0f), m_vulkan.swapChain.extent.width / float(m_vulkan.swapChain.extent.height), 0.1f, 10.0f);
    ubo.proj[1][1] = -1;

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
        vk::ClearValue clearValue;
        clearValue.color.float32[3] = 1.0;
        
        vk::RenderPassBeginInfo rpBeginInfo(*m_vulkan.renderPass, *m_vulkan.swapChain.frameBuffers[i],  renderArea, 1, &clearValue);
        m_vulkan.commandBuffers[i]->beginRenderPass(rpBeginInfo, vk::SubpassContents::eInline);
        m_vulkan.commandBuffers[i]->bindPipeline(vk::PipelineBindPoint::eGraphics, *m_vulkan.pipeLine);
        vk::DeviceSize offset =  0;
        m_vulkan.commandBuffers[i]->bindVertexBuffers(0, 1, &*m_vulkan.vertexBuffer, &offset);
        m_vulkan.commandBuffers[i]->bindIndexBuffer(*m_vulkan.indexBuffer, offset, vk::IndexType::eUint16);
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
        recreateSwapChain();
        return;
    } catch (vk::SystemError e) {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

}


void vkRender::copyBuffer(vk::UniqueBuffer& srcBuffer, vk::UniqueBuffer& dstBuffer, vk::DeviceSize size)
{
    vk::CommandBufferAllocateInfo allocInfo;
    allocInfo.setLevel(vk::CommandBufferLevel::ePrimary).setCommandBufferCount(1).setCommandPool(*m_vulkan.commandPool);
    auto commandBuffer = m_vulkan.device->allocateCommandBuffersUnique(allocInfo);
    commandBuffer[0]->begin(vk::CommandBufferBeginInfo().setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
    commandBuffer[0]->copyBuffer(*srcBuffer, *dstBuffer, vk::BufferCopy().setSize(size));
    commandBuffer[0]->end();
    
    vk::SubmitInfo submitInfo;
    submitInfo.setCommandBufferCount(1).setPCommandBuffers(&*commandBuffer[0]);

    m_vulkan.gQueue.queue.submit(1, &submitInfo, nullptr);
    m_vulkan.gQueue.queue.waitIdle();
}

void vkRender::createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties, vk::UniqueBuffer& buffer, vk::UniqueDeviceMemory& bufferMemory)
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
