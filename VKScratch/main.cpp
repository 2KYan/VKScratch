/*
 * Vulkan Windowed Program
 *
 * Copyright (C) 2016, 2018 Valve Corporation
 * Copyright (C) 2016, 2018 LunarG, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
Vulkan C++ Windowed Project Template
Create and destroy a Vulkan surface on an SDL window.
*/

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

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vulkan/vulkan.hpp>
#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#include <SDL2/SDL_vulkan.h>

#include <iostream>
#include <vector>

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
    return extensions;
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

uint32_t m_width = 1280;
uint32_t m_height = 720;

void initVulkan()
{

}

void initWindow()
{

}

int main()
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

    std::vector<const char*> layers;
    std::vector<const char*> extensions = getInstanceExtensions();

    // Use validation layers if this is a debug build
    #if defined(_DEBUG)
    extensions.push_back("VK_EXT_debug_utils");
    layers.push_back("VK_LAYER_LUNARG_standard_validation");
    #endif

    // vk::ApplicationInfo allows the programmer to specifiy some basic information about the
    // program, which can be useful for layers and tools to provide more debug information.
    vk::ApplicationInfo appInfo = vk::ApplicationInfo()
    .setPApplicationName("Vulkan C++ Windowed Program Template")
    .setApplicationVersion(1)
    .setPEngineName("LunarG SDK")
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

    // Create the Vulkan instance.
    try {
        vk::UniqueInstance instance = vk::createInstanceUnique(instInfo);
        vk::DispatchLoaderDynamic dldi(*instance);

        VkSurfaceKHR c_surface;
        if (!SDL_Vulkan_CreateSurface(window, static_cast<VkInstance>(*instance), &c_surface)) {
            std::cout << "Could not create a Vulkan surface." << std::endl;
            return 1;
        }
        vk::SurfaceKHR surface(c_surface);

        vk::DebugUtilsMessengerCreateInfoEXT debugCreateInfo = vk::DebugUtilsMessengerCreateInfoEXT()
            .setMessageSeverity(vk::DebugUtilsMessageSeverityFlagBitsEXT::eError | vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning)
            .setMessageType(vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance)
            .setPfnUserCallback(debugCallback)
            .setPUserData(nullptr);
        auto dbgMessenger = instance->createDebugUtilsMessengerEXTUnique(debugCreateInfo, nullptr, dldi);
        // This is where most initializtion for a program should be performed
        std::vector<vk::PhysicalDevice> devices = instance->enumeratePhysicalDevices(dldi);

        vk::PhysicalDevice physicalDevice;
        uint32_t pqIdx = -1;
        uint32_t gqIdx = -1;
        for (auto dev : devices) {
            vk::PhysicalDeviceProperties deviceProperties = dev.getProperties();
            vk::PhysicalDeviceFeatures deviceFeatures = dev.getFeatures();
            //vk::PhysicalDeviceProperties2 deviceProperties2 = device.getProperties2(dldi);
            //vk::PhysicalDeviceFeatures2 deviceFeatures2 = device.getFeatures2(dldi);
            if (deviceProperties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu || deviceProperties.deviceType == vk::PhysicalDeviceType::eIntegratedGpu) {
                physicalDevice = dev;
            } else {
                continue;
            }

            std::vector<vk::QueueFamilyProperties> qFamilies = physicalDevice.getQueueFamilyProperties();

            for (auto& qFamily : qFamilies) {
                uint32_t qIdx = &qFamily - &qFamilies[0];
                if (qFamily.queueFlags & vk::QueueFlagBits::eGraphics) {
                    gqIdx = qIdx;
                    if (physicalDevice.getSurfaceSupportKHR(qIdx, surface)) {
                        pqIdx = qIdx;
                        break;
                    }
                }
                if (physicalDevice.getSurfaceSupportKHR(qIdx, surface)) {
                    pqIdx = qIdx;
                }
            }
            if (pqIdx >= qFamilies.size() || gqIdx >= qFamilies.size()) {
                continue;
            }
        }
        
        if (pqIdx == -1 || gqIdx == -1) {
            throw std::runtime_error("Can not find correct Queue");
        }

        float queuePriority = 0.0f;
        vk::DeviceQueueCreateInfo deviceQueueCreateInfo(vk::DeviceQueueCreateFlags(), gqIdx, 1, &queuePriority);
        vk::DeviceCreateInfo deviceCreateInfo = vk::DeviceCreateInfo(vk::DeviceCreateFlags(), 1, &deviceQueueCreateInfo);
        //device = physicalDevice.createDeviceUnique(vk::DeviceCreateInfo(vk::DeviceCreateFlags(), 1, &deviceQueueCreateInfo));
        auto deviceExtensions = getDeviceExtensions();
        deviceCreateInfo.setEnabledExtensionCount(deviceExtensions.size());
        deviceCreateInfo.setPpEnabledExtensionNames(deviceExtensions.data());
        vk::UniqueDevice device = physicalDevice.createDeviceUnique(deviceCreateInfo);

        vk::CommandPoolCreateInfo cmdPoolCreateInfo = vk::CommandPoolCreateInfo().setQueueFamilyIndex(deviceQueueCreateInfo.queueFamilyIndex);
        vk::UniqueCommandPool cmdPool;
        cmdPool = device->createCommandPoolUnique(cmdPoolCreateInfo);
        std::vector<vk::UniqueCommandBuffer> cmdBuffers = device->allocateCommandBuffersUnique(vk::CommandBufferAllocateInfo(cmdPool.get()).setCommandBufferCount(1));

        //Swap Chain
        // The FIFO present mode is guaranteed by the spec to be supported
        vk::PresentModeKHR swapchainPresentMode = vk::PresentModeKHR::eFifo;
        vk::SurfaceCapabilitiesKHR surfaceCapabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface);
        std::vector<vk::SurfaceFormatKHR> formats = physicalDevice.getSurfaceFormatsKHR(surface);
        assert(!formats.empty());
        vk::Format format = (formats[0].format == vk::Format::eUndefined) ? vk::Format::eB8G8R8A8Unorm : formats[0].format;

        VkExtent2D swapchainExtent = surfaceCapabilities.currentExtent;

        vk::SurfaceTransformFlagBitsKHR preTransform = (surfaceCapabilities.supportedTransforms & vk::SurfaceTransformFlagBitsKHR::eIdentity) ? vk::SurfaceTransformFlagBitsKHR::eIdentity : surfaceCapabilities.currentTransform;

        vk::CompositeAlphaFlagBitsKHR compositeAlpha =
            (surfaceCapabilities.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::ePreMultiplied) ? vk::CompositeAlphaFlagBitsKHR::ePreMultiplied :
            (surfaceCapabilities.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::ePostMultiplied) ? vk::CompositeAlphaFlagBitsKHR::ePostMultiplied :
            (surfaceCapabilities.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::eInherit) ? vk::CompositeAlphaFlagBitsKHR::eInherit : vk::CompositeAlphaFlagBitsKHR::eOpaque;

        vk::SwapchainCreateInfoKHR swapChainCreateInfo(vk::SwapchainCreateFlagsKHR(), surface, surfaceCapabilities.minImageCount, format, vk::ColorSpaceKHR::eSrgbNonlinear,
            swapchainExtent, 1, vk::ImageUsageFlagBits::eColorAttachment, vk::SharingMode::eExclusive, 0, nullptr, preTransform, compositeAlpha, swapchainPresentMode, true, nullptr);

        uint32_t queueFamilyIndices[2] = { gqIdx, pqIdx };
        if (gqIdx != pqIdx) {
            // If the graphics and present queues are from different queue families, we either have to explicitly transfer ownership of images between
            // the queues, or we have to create the swapchain with imageSharingMode as VK_SHARING_MODE_CONCURRENT
            swapChainCreateInfo.imageSharingMode = vk::SharingMode::eConcurrent;
            swapChainCreateInfo.queueFamilyIndexCount = 2;
            swapChainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
        }

        vk::UniqueSwapchainKHR swapChain = device->createSwapchainKHRUnique(swapChainCreateInfo);

        std::vector<vk::Image> swapChainImages = device->getSwapchainImagesKHR(swapChain.get());

        std::vector<vk::UniqueImageView> imageViews;
        imageViews.reserve(swapChainImages.size());
        for (auto image : swapChainImages) {
            vk::ComponentMapping componentMapping(vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eG, vk::ComponentSwizzle::eB, vk::ComponentSwizzle::eA);
            vk::ImageSubresourceRange subResourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);
            vk::ImageViewCreateInfo imageViewCreateInfo(vk::ImageViewCreateFlags(), image, vk::ImageViewType::e2D, format, componentMapping, subResourceRange);
            imageViews.push_back(device->createImageViewUnique(imageViewCreateInfo));
        }

        //Depth buffer
        const vk::Format depthFormat = vk::Format::eD16Unorm;
        vk::FormatProperties formatProperties = physicalDevice.getFormatProperties(depthFormat);

        vk::ImageTiling tiling;
        if (formatProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment) {
            tiling = vk::ImageTiling::eOptimal;
        } else if (formatProperties.linearTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment) {
            tiling = vk::ImageTiling::eLinear;
        } else {
            throw std::runtime_error("DepthStencilAttachment is not supported for D16Unorm depth format.");
        }
        vk::ImageCreateInfo imageCreateInfo(vk::ImageCreateFlags(), vk::ImageType::e2D, depthFormat, vk::Extent3D(m_width, m_height, 1), 1, 1, vk::SampleCountFlagBits::e1, tiling, vk::ImageUsageFlagBits::eDepthStencilAttachment);
        vk::UniqueImage depthImage = device->createImageUnique(imageCreateInfo);

        vk::PhysicalDeviceMemoryProperties imageMemoryProperties = physicalDevice.getMemoryProperties();
        vk::MemoryRequirements imageMemoryRequirement = device->getImageMemoryRequirements(depthImage.get());

        uint32_t typeBits = imageMemoryRequirement.memoryTypeBits;
        uint32_t typeIndex = -1;
        for (uint32_t i = 0; i < imageMemoryProperties.memoryTypeCount; ++i) {
            if ((typeBits & 1) && (imageMemoryProperties.memoryTypes[i].propertyFlags & vk::MemoryPropertyFlagBits::eDeviceLocal)) {
                typeIndex = i;
                break;
            }
            typeBits >>= 1;
        }
        assert(typeIndex != -1);
        vk::UniqueDeviceMemory depthMemory = device->allocateMemoryUnique(vk::MemoryAllocateInfo(imageMemoryRequirement.size, typeIndex));
        device->bindImageMemory(depthImage.get(), depthMemory.get(), 0);

        vk::ComponentMapping componentMapping(vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eG, vk::ComponentSwizzle::eB, vk::ComponentSwizzle::eA);
        vk::ImageSubresourceRange subResourceRange(vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1);
        vk::UniqueImageView depthView = device->createImageViewUnique(vk::ImageViewCreateInfo(vk::ImageViewCreateFlags(), depthImage.get(), vk::ImageViewType::e2D, depthFormat, componentMapping, subResourceRange));

        //Uniform Buffer
        glm::mat4x4 projection = glm::perspective(glm::radians(45.0f), 1.0f, 0.1f, 100.0f);
        glm::mat4x4 view = glm::lookAt(glm::vec3(-5.0f, 3.0f, -10.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f));
        glm::mat4x4 model = glm::mat4x4(1.0f);
        glm::mat4x4 clip = glm::mat4x4(1.0f, 0.0f, 0.0f, 0.0f,
                                       0.0f, -1.0f, 0.0f, 0.0f,
                                       0.0f, 0.0f, 0.5f, 0.0f,
                                       0.0f, 0.0f, 0.5f, 1.0f);
        glm::mat4x4 mvpc = clip * projection * view * model;

        vk::UniqueBuffer uniformDataBuffer = device->createBufferUnique(vk::BufferCreateInfo(vk::BufferCreateFlags(), sizeof(mvpc), vk::BufferUsageFlagBits::eUniformBuffer));
        vk::MemoryRequirements dataMemoryRequirement = device->getBufferMemoryRequirements(uniformDataBuffer.get());

        vk::PhysicalDeviceMemoryProperties dataMemoryProperties = physicalDevice.getMemoryProperties();
        vk::MemoryPropertyFlags dataMemoryRequirementMask = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;

        typeBits = dataMemoryRequirement.memoryTypeBits;
        typeIndex = -1;
        for (uint32_t i = 0; i < dataMemoryProperties.memoryTypeCount; ++i) {
            if ((typeBits & 1) && (dataMemoryProperties.memoryTypes[i].propertyFlags & dataMemoryRequirementMask)) {
                typeIndex = i;
                break;
            }
            typeBits >>= 1;
        }
        assert(typeIndex != -1);
        vk::UniqueDeviceMemory uniformDataMemory = device->allocateMemoryUnique(vk::MemoryAllocateInfo(dataMemoryRequirement.size, typeIndex));
        uint8_t* pData = static_cast<uint8_t*>(device->mapMemory(uniformDataMemory.get(), 0, dataMemoryRequirement.size));
        memcpy(pData, &mvpc, sizeof(mvpc));
        device->unmapMemory(uniformDataMemory.get());

        device->bindBufferMemory(uniformDataBuffer.get(), uniformDataMemory.get(), 0);

        //Descriptor Set
        vk::DescriptorSetLayoutBinding dsLayoutBinding = vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex);
        vk::UniqueDescriptorSetLayout dsLayout = device->createDescriptorSetLayoutUnique(vk::DescriptorSetLayoutCreateInfo(vk::DescriptorSetLayoutCreateFlags(), 1, &dsLayoutBinding));

        vk::DescriptorPoolSize poolSize(vk::DescriptorType::eUniformBuffer, 1);
        vk::UniqueDescriptorPool dsPool = device->createDescriptorPoolUnique(vk::DescriptorPoolCreateInfo({}, 1, 1, &poolSize));
        std::vector<vk::UniqueDescriptorSet> ds = device->allocateDescriptorSetsUnique(vk::DescriptorSetAllocateInfo(dsPool.get(), 1, &dsLayout.get()));

        vk::DescriptorBufferInfo dsBufferInfo(uniformDataBuffer.get(), 0, sizeof(glm::mat4x4));
        device->updateDescriptorSets(vk::WriteDescriptorSet(ds[0].get(), 0, 0, 1, vk::DescriptorType::eUniformBuffer, nullptr, &dsBufferInfo), {});

        vk::UniquePipelineLayout plLayout = device->createPipelineLayoutUnique(vk::PipelineLayoutCreateInfo(vk::PipelineLayoutCreateFlags(), 1, &dsLayout.get()));

        //REndering pass
        vk::AttachmentDescription attachments[2];
        attachments[0] = vk::AttachmentDescription(vk::AttachmentDescriptionFlags(), format, vk::SampleCountFlagBits::e1, vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore,
                                                   vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare, vk::ImageLayout::eUndefined, vk::ImageLayout::ePresentSrcKHR);
        attachments[1] = vk::AttachmentDescription(vk::AttachmentDescriptionFlags(), depthFormat, vk::SampleCountFlagBits::e1, vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eDontCare,
                                                   vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare, vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthStencilAttachmentOptimal);

        vk::AttachmentReference colorReference(0, vk::ImageLayout::eColorAttachmentOptimal);
        vk::AttachmentReference depthReference(1, vk::ImageLayout::eDepthStencilAttachmentOptimal);
        vk::SubpassDescription subpass(vk::SubpassDescriptionFlags(), vk::PipelineBindPoint::eGraphics, 0, nullptr, 1, &colorReference, nullptr, &depthReference);

        vk::UniqueRenderPass renderPass = device->createRenderPassUnique(vk::RenderPassCreateInfo(vk::RenderPassCreateFlags(), 2, attachments, 1, &subpass));


        // Poll for user input.
        bool stillRunning = true;
        while (stillRunning) {

            SDL_Event event;
            while (SDL_PollEvent(&event)) {

                switch (event.type) {

                case SDL_QUIT:
                    stillRunning = false;
                    break;

                default:
                    // Do nothing.
                    break;
                }
            }

            SDL_Delay(10);
        }

        // Clean up.
        SDL_DestroyWindow(window);
        SDL_Quit();
    }
    catch (vk::SystemError err)
    {
        std::cout << "vk::SystemError: " << err.what() << std::endl;
        exit(-1);
    }
    catch (std::runtime_error err) 
    {
        std::cout << "Runtime Error: " << err.what() << std::endl;
        exit(-1);
    }
    catch (...)
    {
        std::cout << "unknown error\n";
        exit(-1);
    }
    return 0;
}
