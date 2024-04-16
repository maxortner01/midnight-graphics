#include <iostream>

#include <memory>
#include <exception>
#include <cassert>
#include <vector>
#include <algorithm>
#include <span>

#include <vulkan/vulkan.h>
#include <SL/Lua.hpp>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#ifndef SOURCE_DIR
#define SOURCE_DIR "."
#endif

using handle_t = void*;

template<typename T>
struct Singleton
{
    friend T;

    inline static std::shared_ptr<T> get()
    {
        if (!_instance) _instance = 
            std::shared_ptr<T>(
                new T(),
                [](void* ptr) { delete reinterpret_cast<T*>(ptr); }
            );
        return _instance;
    }

    inline static void destroy()
    {
        _instance.reset();
    }

private:
    inline static std::shared_ptr<T> _instance;
};

struct Instance : Singleton<Instance>
{
    friend class Singleton<Instance>;

    handle_t createSurface(handle_t window) const
    {
        if (!handle) std::terminate();

        VkSurfaceKHR surface;
        const auto err = SDL_Vulkan_CreateSurface(
            static_cast<SDL_Window*>(window),
            static_cast<VkInstance>(handle),
            nullptr,
            &surface
        );

        if (err == SDL_FALSE)
        {
            std::cout << "Error creating surface: " << SDL_GetError() << std::endl;
            std::terminate();
        }

        return static_cast<handle_t>(surface);
    }

    void destroySurface(handle_t surface) const
    {
        if (!handle) std::terminate();
        vkDestroySurfaceKHR(static_cast<VkInstance>(handle), static_cast<VkSurfaceKHR>(surface), nullptr);
    }

    std::pair<handle_t, std::vector<handle_t>> createSwapchain(handle_t window, handle_t surface) const
    {
        if (!physical_device) std::terminate();
        auto* win = static_cast<SDL_Window*>(window);
        const auto  p_device   = static_cast<VkPhysicalDevice>(physical_device);
        const auto  vk_surface = static_cast<VkSurfaceKHR>(surface); 

        VkSurfaceCapabilitiesKHR capabilities;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(p_device, vk_surface, &capabilities);

        const auto surfaceFormats = [p_device, vk_surface]()
        {
            uint32_t count;
            vkGetPhysicalDeviceSurfaceFormatsKHR(p_device, vk_surface, &count, nullptr);
            std::vector<VkSurfaceFormatKHR> formats(count);
            vkGetPhysicalDeviceSurfaceFormatsKHR(p_device, vk_surface, &count, formats.data());
            return formats;
        }();

        if (surfaceFormats[0].format != VK_FORMAT_B8G8R8A8_UNORM)
        {
            std::cout << "surfaceFormats[0].format != VK_FORMAT_B8G8R8A8_UNORM" << std::endl;
            std::terminate();
        }

        const auto [w, h] = [win, c = capabilities]()
        {
            int w, h;
            SDL_GetWindowSize(win, &w, &h);
            if (w < 0 || h < 0) std::terminate();
            return std::tuple(
                std::clamp(static_cast<uint32_t>(w), c.minImageExtent.width,  c.maxImageExtent.width ), 
                std::clamp(static_cast<uint32_t>(h), c.minImageExtent.height, c.maxImageExtent.height)
            );
        }();

        uint32_t image_count = capabilities.minImageCount + 1;
        if (capabilities.maxImageCount > 0 && image_count > capabilities.maxImageCount)
            image_count = capabilities.maxImageCount;

        VkSwapchainCreateInfoKHR create_info = {
            .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            .pNext = nullptr,
            .flags = 0,
            .oldSwapchain = nullptr,
            .minImageCount = capabilities.minImageCount,
            .surface = vk_surface,
            .imageExtent = VkExtent2D{ .width = w, .height = h },
            .imageFormat = surfaceFormats[0].format,
            .imageColorSpace = surfaceFormats[0].colorSpace,
            .imageArrayLayers = 1,
            .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            .queueFamilyIndexCount = 1,
            .pQueueFamilyIndices = &graphics_queue_index,
            .preTransform = capabilities.currentTransform,
            .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
            .presentMode = VK_PRESENT_MODE_FIFO_KHR,
            .clipped = VK_TRUE
        };

        VkSwapchainKHR swapchain;
        const auto err = vkCreateSwapchainKHR(static_cast<VkDevice>(device), &create_info, nullptr, &swapchain);
        if (err != VK_SUCCESS)
        {
            std::cout << "Error creating swapchain (" << err << ")" << std::endl;
            std::terminate();
        }

        std::pair<handle_t, std::vector<handle_t>> pair;
        pair.first = static_cast<handle_t>(swapchain);

        const auto images = getSwapchainImages(pair.first);
        
        pair.second.reserve(images.size());
        for (uint32_t i = 0; i < images.size(); i++)
        {
            VkImageViewCreateInfo create_info = {
                .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .pNext = nullptr,
                .image = static_cast<VkImage>(images[i]),
                .viewType = VK_IMAGE_VIEW_TYPE_2D,
                .format   = surfaceFormats[0].format,
                .components = {
                    .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                    .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                    .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                    .a = VK_COMPONENT_SWIZZLE_IDENTITY
                },
                .subresourceRange = {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = 1
                }
            };

            VkImageView image_view;
            const auto err = vkCreateImageView(static_cast<VkDevice>(device), &create_info, nullptr, &image_view);
            if (err != VK_SUCCESS)
                std::terminate();

            pair.second.push_back(static_cast<handle_t>(image_view));
        }

        return pair;
    }

    std::vector<handle_t> getSwapchainImages(handle_t swapchain) const
    {
        assert(sizeof(handle_t) == sizeof(VkImage));
        const auto vk_device = static_cast<VkDevice>(device);
        const auto vk_swap   = static_cast<VkSwapchainKHR>(swapchain);
        uint32_t count;
        vkGetSwapchainImagesKHR(vk_device, vk_swap, &count, nullptr);
        std::vector<handle_t> images(count); 
        vkGetSwapchainImagesKHR(vk_device, vk_swap, &count, reinterpret_cast<VkImage*>(images.data()));
        return images;
    }

    void destroySwapchain(handle_t swapchain) const
    {
        if (!device) std::terminate();
        vkDestroySwapchainKHR(static_cast<VkDevice>(device), static_cast<VkSwapchainKHR>(swapchain), nullptr);
    }

    void destroyImageView(handle_t image_view) const
    {
        if (!device) std::terminate();
        vkDestroyImageView(static_cast<VkDevice>(device), static_cast<VkImageView>(image_view), nullptr);
    }

private:
    Instance() : handle(nullptr)
    {
        VkApplicationInfo app_info = {
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pNext = nullptr,
            .apiVersion = VK_API_VERSION_1_0,
            .engineVersion = VK_MAKE_API_VERSION(0, 1, 0, 0),
            .pEngineName = "midnight",
            .applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0),
            .pApplicationName = "application"
        };

        VkInstanceCreateInfo create_info = {
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pNext = nullptr,
            .flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR,
            .pApplicationInfo = &app_info,
            .enabledLayerCount = 0,
            .ppEnabledLayerNames = nullptr
        };

        std::vector<const char*> enabled_extensions = { 
            VK_KHR_SURFACE_EXTENSION_NAME, 
            VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME, 
            "VK_EXT_metal_surface", 
            VK_EXT_DEBUG_UTILS_EXTENSION_NAME, 
            VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
            VK_KHR_DEVICE_GROUP_CREATION_EXTENSION_NAME
        };

        create_info.enabledExtensionCount   = static_cast<uint32_t>(enabled_extensions.size());
        create_info.ppEnabledExtensionNames = enabled_extensions.data();

        const auto validation_layers = []()
        {
            std::vector<const char*> enabledLayers;
            std::vector<const char*> requiredLayers = { "VK_LAYER_KHRONOS_validation" };

            uint32_t count;
            vkEnumerateInstanceLayerProperties(&count, nullptr);
            std::vector<VkLayerProperties> props(count);
            vkEnumerateInstanceLayerProperties(&count, props.data());

            for (const auto& layer : requiredLayers)
            {
                bool found = false;
                for (const auto& prop : props)
                    if (!strcmp(prop.layerName, layer))
                    {
                        found = true;
                        break;
                    }
                
                if (found) enabledLayers.push_back(layer);
                else std::cout << "Layer " << layer << " not supported\n";
            }

            return enabledLayers;
        }();
        create_info.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
        create_info.ppEnabledLayerNames = validation_layers.data();

        VkInstance instance;
        const auto err = vkCreateInstance(&create_info, nullptr, &instance);
        if (err != VK_SUCCESS)
        {
            std::cout << "Error initializing vulkan (" << err << ")" << std::endl;
            std::terminate();
        }

        std::cout << "Successfully created instance\n";
        handle = static_cast<handle_t>(instance);

        createDevice();
    }

    void createDevice()
    {
        const auto instance = static_cast<VkInstance>(handle);
        if (!instance) std::terminate();

        const auto physical_devices = [](const VkInstance& instance)
        {
            uint32_t count;
            vkEnumeratePhysicalDevices(instance, &count, nullptr);
            std::vector<VkPhysicalDevice> devices(count);
            vkEnumeratePhysicalDevices(instance, &count, devices.data());
            return devices;
        }(instance);

        if (!physical_devices.size())
        {
            std::cout << "No physical devices found" << std::endl;
            std::terminate();
        }

        // We can check if verbose is on or not
        std::cout << "Found " << physical_devices.size() << " physical device(s):\n";
        for (const auto& device : physical_devices)
        {
            VkPhysicalDeviceProperties props;
            vkGetPhysicalDeviceProperties(device, &props);
            std::cout << "\n";
            std::cout << "Device Name: " << props.deviceName << "\n";
            std::cout << "Device Type: ";

            switch (props.deviceType)
            {
            case VK_PHYSICAL_DEVICE_TYPE_CPU:            std::cout << "CPU\n";            break;
            case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:   std::cout << "Discrete GPU\n";   break;
            case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: std::cout << "Integrated GPU\n"; break;
            case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:    std::cout << "Virtual GPU\n";    break;
            case VK_PHYSICAL_DEVICE_TYPE_MAX_ENUM:
            case VK_PHYSICAL_DEVICE_TYPE_OTHER:          std::cout << "Other\n";          break;
            }

            std::cout << "API version: " << VK_VERSION_MAJOR(props.apiVersion) << "." << VK_VERSION_MINOR(props.apiVersion) << "." << VK_VERSION_PATCH(props.apiVersion) << "\n";
        }
        if (physical_devices.size()) std::cout << "\n";

        const auto p_device = physical_devices[0];
        physical_device = static_cast<handle_t>(p_device);

        const auto graphics_index = [](const VkPhysicalDevice& device)
        {
            const auto queue_families = [](const VkPhysicalDevice& device)
            {
                uint32_t count;
                vkGetPhysicalDeviceQueueFamilyProperties(device, &count, nullptr);
                std::vector<VkQueueFamilyProperties> props(count);
                vkGetPhysicalDeviceQueueFamilyProperties(device, &count, props.data());
                return props;
            }(device);

            uint32_t graphics_index = 9999;
            for (uint32_t i = 0; i < queue_families.size(); i++)
                if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
                {
                    graphics_index = i;
                    break;
                }
            
            return graphics_index;
        }(p_device);

        float priority = 1.f;
        VkDeviceQueueCreateInfo queue_create = {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .pNext = nullptr,
            .queueFamilyIndex = graphics_index,
            .queueCount = 1,
            .pQueuePriorities = &priority
        };

        // Get the necessary device extension names
        const auto extensions = [](const VkPhysicalDevice& p_device)
        {
            std::vector<const char*> enabledExtensions;
            std::vector<const char*> requiredExtensions = { 
                VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME, 
                VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME, 
                VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME, 
                VK_KHR_SWAPCHAIN_EXTENSION_NAME,
                VK_KHR_DEPTH_STENCIL_RESOLVE_EXTENSION_NAME,
                VK_KHR_DEVICE_GROUP_EXTENSION_NAME,
                VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME,
                VK_KHR_MULTIVIEW_EXTENSION_NAME,
                VK_KHR_MAINTENANCE2_EXTENSION_NAME,
                "VK_KHR_portability_subset"
            };

            uint32_t count;
            vkEnumerateDeviceExtensionProperties(p_device, nullptr, &count, nullptr);
            std::vector<VkExtensionProperties> extensions(count);
            vkEnumerateDeviceExtensionProperties(p_device, nullptr, &count, extensions.data());
            // Go through required extensions and make sure they are in extensions
            for (const auto& required : requiredExtensions)
            {
                bool found = false;
                for (const auto& ext : extensions)
                    if (!strcmp(ext.extensionName, required))
                    {
                        found = true;
                        break;
                    }

                if (!found)
                {
                    std::cout << "Extension (" << required << ") not supported by this physical device" << std::endl;
                    std::terminate();
                }

                enabledExtensions.push_back(required);
            }

            return enabledExtensions;
        }(p_device);

        VkDeviceCreateInfo create_info = {
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .queueCreateInfoCount = 1,
            .pQueueCreateInfos = &queue_create,
            .enabledExtensionCount   = static_cast<uint32_t>(extensions.size()),
            .ppEnabledExtensionNames = extensions.data(),
            .enabledLayerCount = 0,
            .ppEnabledLayerNames = nullptr
        };

        VkDevice _device;
        const auto err = vkCreateDevice(p_device, &create_info, nullptr, &_device);
        if (err != VK_SUCCESS)
        {
            std::cout << "Error creating device (" << err << ")" << std::endl;
            std::terminate();
        }

        std::cout << "Successfully created device\n";
        device = static_cast<handle_t>(_device);
        graphics_queue_index = graphics_index;
    }

    ~Instance()
    {
        if (handle)
        {
            vkDestroyInstance(static_cast<VkInstance>(handle), nullptr);
            handle = nullptr;
        }
        std::cout << "Destroy\n";
    }

    handle_t handle, physical_device, device;
    uint32_t graphics_queue_index;
};

struct Window
{
    Window(const std::string& config_file = "")
    {
        if (SDL_WasInit(SDL_INIT_VIDEO)) std::terminate();

        // Initialize SDL
        if (SDL_Init(SDL_INIT_VIDEO) != 0)
        {
            std::cout << "Error initializing video\n";
            std::terminate();
        }

        // Read in the window information from the file
        const auto [title, width, height] = (config_file.length() ? [](const std::string& file)
        {
            SL::Runtime runtime(SOURCE_DIR "/" + file);
            const auto res = runtime.getGlobal<SL::Table>("WindowOptions");
            assert(res);

            const auto string = res->get<SL::String>("title");
            const auto w = res->get<SL::Table>("size").get<SL::Number>("w");
            const auto h = res->get<SL::Table>("size").get<SL::Number>("h");
            return std::tuple(string, w, h);
        }(config_file) : std::tuple("window", 1280, 720) );

        // Create the window
        handle = static_cast<handle_t>(SDL_CreateWindow(title.c_str(), width, height, SDL_WINDOW_VULKAN));
        if (!handle)
        {
            std::cout << "Error initializing window\n";
            std::terminate();
        }

        auto instance = Instance::get();
        surface   = instance->createSurface(handle);
        std::tie(swapchain, image_views) = instance->createSwapchain(handle, surface);

        _close = false;
    }

    void close()
    {
        _close = true;
    }

    bool shouldClose() const { return _close; }

    ~Window()
    {
        if (handle)
        {
            {
                auto instance = Instance::get();
                for (const auto& iv : image_views)
                    instance->destroyImageView(iv);   

                instance->destroySwapchain(swapchain);
                instance->destroySurface(surface);
            }
            Instance::destroy();
            SDL_DestroyWindow(static_cast<SDL_Window*>(handle));
            SDL_Quit();
            handle = nullptr;
        }
    }

    void update()
    {
        SDL_UpdateWindowSurface(static_cast<SDL_Window*>(handle));
    }

private:
    bool _close;
    handle_t handle, surface, swapchain;
    std::vector<handle_t> image_views;
};

int main()
{
    Window window("window.lua");

    // Main loop
    while (!window.shouldClose())
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED)
                window.close();
        }

        window.update();
    }
}
