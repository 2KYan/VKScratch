#pragma once

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
#include <vulkan/vulkan.hpp>

#include <iostream>
#include <vector>

struct QueueParams
{
    vk::Queue queue;
    uint32_t familyIndex;
};

struct ImageParams
{
    vk::UniqueImage image;
    vk::UniqueImageView view;
    vk::UniqueSampler sampler;
    vk::UniqueDeviceMemory memory;
};

struct BufferParams
{
    vk::UniqueBuffer buffer;
    vk::UniqueDeviceMemory memory;
    void *pointer;
    uint32_t size;
};

struct DescriptorSetParams
{
    vk::UniqueDescriptorPool pool;
    vk::UniqueDescriptorSetLayout layout;
    vk::UniqueDescriptorSet set;
};

struct SwapChainSupportDetails
{
    vk::SurfaceCapabilitiesKHR capabilities;
    std::vector<vk::SurfaceFormatKHR> formats;
    std::vector<vk::PresentModeKHR> presentModes;
};

struct SwapChainParams
{
    vk::UniqueSwapchainKHR swapChainKHR;
    vk::Format format;
    std::vector<vk::Image> images;
    std::vector<vk::UniqueImageView> views;
    std::vector<vk::UniqueFramebuffer> frameBuffers;
    vk::Extent2D extent;
    vk::PresentInfoKHR presentMode;
    vk::ImageUsageFlags usageFlags;

};

struct SDL_Window;
namespace vk
{
    using DUniqueDebugUtilsMessengerEXT = vk::UniqueHandle<vk::DebugUtilsMessengerEXT,vk::DispatchLoaderDynamic>;
};

struct CommonParams
{
    SDL_Window* window;
    vk::DispatchLoaderDynamic dldi;
    vk::UniqueInstance instance;

    vk::DebugUtilsMessengerEXT dbgMessenger;
    vk::DUniqueDebugUtilsMessengerEXT udbgMessenger;
    vk::SurfaceKHR surfaceKHR;

    vk::PhysicalDevice physicalDevice;
    vk::UniqueDevice device;

    QueueParams gQueue;
    QueueParams pQueue;
    SwapChainParams swapChain;

    vk::UniqueRenderPass renderPass;
    vk::UniquePipelineLayout pipelineLayout;
    vk::UniquePipeline pipeLine;

    vk::UniqueCommandPool commandPool;
    std::vector<vk::UniqueCommandBuffer> commandBuffers;

    std::vector<vk::UniqueSemaphore> imageAvailableSemaphore;
    std::vector<vk::UniqueSemaphore> renderFinishedSemaphore;
    std::vector<vk::UniqueFence> inFlightFences;
};

class vkRender
{
public:
    vkRender();
    virtual ~vkRender();

    int run();

protected:
    uint32_t m_width = 1280;
    uint32_t m_height = 720;
    uint32_t m_max_frame_in_flight = 2;
    uint32_t m_currentFrame = 0; 
    CommonParams m_vulkan;

protected:
    int initWindow();
    int initVulkan();
    int mainLoop();
    int cleanup();
    int resizeWindow(int32_t width, int32_t height);

private:
    void findQueueFamilies(bool presentSupport);
    
private:
    void drawFrame();

    void createInstance();
    void setupDebugMessenger();
    void createSurface();
    void pickPhysicalDevice();
    void createLogicalDevice();

    void createSwapChain();
    void createImageViews();
    void createRenderPass();
    void createGraphicsPipeline();
    void createFrameBuffers();
    void createCommandPool();
    void createCommandBuffers();
    void createSyncObjects();

    void cleanupSwapChain();
    void recreateSwapChain();

};
