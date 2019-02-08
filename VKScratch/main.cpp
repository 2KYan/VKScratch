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
#include <vulkan/vulkan.hpp>
#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#include <SDL2/SDL_vulkan.h>

#include <iostream>
#include <vector>

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

int main()
{
    // Create an SDL window that supports Vulkan rendering.
    if(SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cout << "Could not initialize SDL." << std::endl;
        return 1;
    }
    SDL_Window* window = SDL_CreateWindow("Vulkan Window", SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_VULKAN);
    if(window == NULL) {
        std::cout << "Could not create SDL window." << std::endl;
        return 1;
    }
    
    // Get WSI extensions from SDL (we can add more if we like - we just can't remove these)
    unsigned extension_count;
    if(!SDL_Vulkan_GetInstanceExtensions(window, &extension_count, NULL)) {
        std::cout << "Could not get the number of required instance extensions from SDL." << std::endl;
        return 1;
    }
    std::vector<const char*> extensions(extension_count);
    if(!SDL_Vulkan_GetInstanceExtensions(window, &extension_count, extensions.data())) {
        std::cout << "Could not get the names of required instance extensions from SDL." << std::endl;
        return 1;
    }
    extensions.push_back("VK_EXT_debug_utils");

    std::vector<vk::ExtensionProperties> availExtensions = vk::enumerateInstanceExtensionProperties();
    std::vector<vk::LayerProperties> availableLayers = vk::enumerateInstanceLayerProperties();

    // Use validation layers if this is a debug build
    std::vector<const char*> layers;
    //layers.push_back("VK_EXT_debug_utils");
#if defined(_DEBUG)
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
        VkSurfaceKHR c_surface;
        if(!SDL_Vulkan_CreateSurface(window, static_cast<VkInstance>(*instance), &c_surface)) {
            std::cout << "Could not create a Vulkan surface." << std::endl;
            return 1;
        }
        vk::SurfaceKHR surface(c_surface);

        vk::DispatchLoaderDynamic dldi(*instance);

        vk::DebugUtilsMessengerEXT debugMessenger;
        vk::DebugUtilsMessengerCreateInfoEXT debugCreateInfo = vk::DebugUtilsMessengerCreateInfoEXT()
            .setMessageSeverity(vk::DebugUtilsMessageSeverityFlagBitsEXT::eError | vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning)
            .setMessageType(vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance)
            .setPfnUserCallback(debugCallback)
            .setPUserData(nullptr);

        auto dbgMessenger = instance->createDebugUtilsMessengerEXTUnique(debugCreateInfo, nullptr, dldi);
        // This is where most initializtion for a program should be performed
        std::vector<vk::PhysicalDevice> devices = instance->enumeratePhysicalDevices(dldi);

        for (auto device : devices) {
            vk::PhysicalDeviceProperties deviceProperties = device.getProperties();
            vk::PhysicalDeviceFeatures deviceFeatures = device.getFeatures();
            //vk::PhysicalDeviceProperties2 deviceProperties2 = device.getProperties2(dldi);
            //vk::PhysicalDeviceFeatures2 deviceFeatures2 = device.getFeatures2(dldi);
            printf("test");
        }

        // Poll for user input.
        bool stillRunning = true;
        while(stillRunning) {

            SDL_Event event;
            while(SDL_PollEvent(&event)) {

                switch(event.type) {

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
    catch (...)
    {
        std::cout << "unknown error\n";
        exit(-1);
    }
    return 0;
    // Create a Vulkan surface for rendering
    return 0;
}
