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

struct SwapChainParams
{
    vk::UniqueSwapchainKHR m_swapChainKHR;
    vk::Format foramt;
    std::vector<vk::UniqueImage> images;
    std::vector<vk::UniqueImageView> views;
    vk::Extent2D extent;
    vk::PresentInfoKHR presentMode;
    vk::ImageUsageFlags usageFlags;

};

struct CommonParams
{
    vk::DispatchLoaderDynamic dldi;
    vk::UniqueInstance instance;
    vk::UniqueSurfaceKHR surfaceKHR;

    std::vector<vk::PhysicalDevice> devices;
    uint32_t physicalDeviceIndex;

    vk::UniqueDevice device;

    QueueParams gQueue;
    QueueParams pQueue;
    SwapChainParams swapChain;

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
    CommonParams m_vulkan;

protected:
    int initWindow();
    int initVulkan();
    int mainLoop();
    int cleanup();

private:
    void createInstance();
    void setupDebugMessenger();
    void createSurface();
    void pickPhysicalDevice();
    void createLogicalDevice();
    void createSwapChain();
    void createImageViews();

};
