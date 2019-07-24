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

#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/hash.hpp"

#include <vulkan/vulkan.hpp>

#include <iostream>
#include <vector>

struct Vertex
{
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 texCoord;

    static vk::VertexInputBindingDescription getBindingDescription()
    {
        vk::VertexInputBindingDescription bindingDesc;
        bindingDesc.setBinding(0).setInputRate(vk::VertexInputRate::eVertex).setStride(sizeof(Vertex));
        
        return bindingDesc;
    }

    static std::array<vk::VertexInputAttributeDescription, 3> getAttributeDescription()
    {
        std::array<vk::VertexInputAttributeDescription, 3> attrDesc;
        attrDesc[0].setBinding(0).setLocation(0).setOffset(offsetof(Vertex, pos)).setFormat(vk::Format::eR32G32B32Sfloat);
        attrDesc[1].setBinding(0).setLocation(1).setOffset(offsetof(Vertex, color)).setFormat(vk::Format::eR32G32B32Sfloat);
        attrDesc[2].setBinding(0).setLocation(2).setOffset(offsetof(Vertex, texCoord)).setFormat(vk::Format::eR32G32Sfloat);

        return attrDesc;
    }

    bool operator==(const Vertex& other) const
    {
        return( pos == other.pos && color == other.color && texCoord == other.texCoord);
    }
};

namespace std
{
    template<> struct hash<Vertex>
    {
        size_t operator()(Vertex const& vertex) const
        {
            return ((hash<glm::vec3>()(vertex.pos) ^ (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^ (hash<glm::vec2>()(vertex.texCoord) << 1);
        }
    };
}

struct UniformBufferObject
{
    glm::vec2 foo;
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
};

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

    vk::UniqueBuffer vertexBuffer;
    vk::UniqueDeviceMemory vertexBufferMemory;

    vk::UniqueBuffer indexBuffer;
    vk::UniqueDeviceMemory indexBufferMemory;

    std::vector<vk::UniqueBuffer> uniformBuffer;
    std::vector<vk::UniqueDeviceMemory> uniformBufferMemory;

    vk::UniqueDescriptorSetLayout descriptorsetLayout;
    vk::UniqueDescriptorPool descriptorPool;
    std::vector<vk::UniqueDescriptorSet> descriptorSets;

    vk::UniqueImage depthImage;
    vk::UniqueImageView depthImageView;
    vk::UniqueDeviceMemory depthImageMemory;

    vk::SampleCountFlagBits sampleCount;
    vk::UniqueImage colorImage;
    vk::UniqueImageView colorImageView;
    vk::UniqueDeviceMemory colorImageMemory;

    uint32_t mipLevels;

    vk::UniqueImage textureImage;
    vk::UniqueImageView textureImageView;
    vk::UniqueDeviceMemory textureImageMemory;
    vk::UniqueSampler textureSampler;
   
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
};


class Camera;

class vkRender
{
public:
    vkRender(SDL_Window* window, std::shared_ptr<Camera> pTrackBall, uint32_t width, uint32_t height);
    virtual ~vkRender();

    int initVulkan(uint32_t width, uint32_t height);
    int resizeWindow(int32_t width, int32_t height);
    void waitIdle();
    void drawFrame();

protected:
    uint32_t m_width;
    uint32_t m_height;
    uint32_t m_max_frame_in_flight = 2;
    uint32_t m_currentFrame = 0; 
    CommonParams m_vulkan;
    std::shared_ptr<Camera> m_pCamera;

protected:
    void loadModel();

    void updateUniformBuffer(uint32_t index);

    void createInstance();
    void setupDebugMessenger();
    void createSurface();
    void pickPhysicalDevice();
    void createLogicalDevice();

    void createSwapChain(uint32_t width, uint32_t height);
    void createRenderPass();
    void createDescriptorSetLayout();
    void createGraphicsPipeline();
    void createFrameBuffers();

    void createCommandPool();

    void createDepthResources();
    void createColorResources();

    void createTextureImage();
    void createTextureSampler();

    void createVertexBuffer();
    void createIndexBuffer();

    void createUniformBuffer();

    void createDescriptorPool();
    void createDescriptorSets();
    void createCommandBuffers();

    void createSyncObjects();

    void cleanupSwapChain();
    void recreateSwapChain(uint32_t width, uint32_t height);

    template <typename... Args>
    void submit(void (vkRender::*func)(Args...), Args... args);

protected:
    void findQueueFamilies(bool presentSupport);
    std::vector<vk::UniqueCommandBuffer> beginSingleTimeCommands();
    void endSingleTimeCommands(std::vector<vk::UniqueCommandBuffer>& commandBuffers);

    void transitionImageLayout(vk::UniqueImage& image, vk::Format format, vk::ImageLayout oldLayout, vk::ImageLayout newLayout, uint32_t mipLevels);

    void utilCreateBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties, vk::UniqueBuffer& buffer, vk::UniqueDeviceMemory& bufferMemory);
    void utilCreateImage(uint32_t width, uint32_t height, uint32_t mipLevels, vk::SampleCountFlagBits msaa, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties, vk::UniqueImage& image, vk::UniqueDeviceMemory& imageMemory);
    vk::UniqueImageView utilCreateImageView(vk::Image& image, vk::Format format, vk::ImageAspectFlags aspectFlags, uint32_t mipLevels);
    void generateMipmaps(vk::Image& image, vk::Format format, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);

    void copyBufferToImage(vk::UniqueBuffer& buffer, vk::UniqueImage& image, uint32_t width, uint32_t height);
    void copyBuffer(vk::UniqueBuffer& srcBuffer, vk::UniqueBuffer& dstBuffer, vk::DeviceSize size);

    vk::Format findDepthFormat();
    uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties);
    vk::Format findSupportedFormat(const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features);
    bool hasStencilComponent(vk::Format format);

    vk::SampleCountFlagBits getMaxUsableSampleCount();
};
