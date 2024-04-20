#include <Graphics/Backend/Instance.hpp>
#include <Graphics/Window.hpp>

#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

namespace mn::Graphics::Backend
{

using handle_t = mn::handle_t;

handle_t Instance::createSurface(Handle<Window> window) const
{
    if (!*handle) std::terminate();

    VkSurfaceKHR surface;
    const auto err = SDL_Vulkan_CreateSurface(
        window.as<SDL_Window*>(),
        handle.as<VkInstance>(),
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

void Instance::destroySurface(handle_t surface) const
{
    if (!*handle) std::terminate();
    vkDestroySurfaceKHR(handle.as<VkInstance>(), static_cast<VkSurfaceKHR>(surface), nullptr);
}

std::pair<handle_t, std::vector<handle_t>> Instance::createSwapchain(Handle<Window> window, handle_t surface) const
{
    if (!device || !device->getHandle()) std::terminate();
    auto* win = window.as<SDL_Window*>();
    const auto  p_device   = static_cast<VkPhysicalDevice>(device->getPhysicalDevice());
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

    const auto graphics_queue = device->getGraphicsQueue();

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
        .pQueueFamilyIndices = &graphics_queue.index,
        .preTransform = capabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = VK_PRESENT_MODE_FIFO_KHR,
        .clipped = VK_TRUE
    };

    VkSwapchainKHR swapchain;
    const auto err = vkCreateSwapchainKHR(device->getHandle().as<VkDevice>(), &create_info, nullptr, &swapchain);
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
        const auto err = vkCreateImageView(device->getHandle().as<VkDevice>(), &create_info, nullptr, &image_view);
        if (err != VK_SUCCESS)
            std::terminate();

        pair.second.push_back(static_cast<handle_t>(image_view));
    }

    return pair;
}

std::vector<handle_t> Instance::getSwapchainImages(handle_t swapchain) const
{
    assert(sizeof(handle_t) == sizeof(VkImage));
    const auto vk_device = device->getHandle().as<VkDevice>();
    const auto vk_swap   = static_cast<VkSwapchainKHR>(swapchain);
    uint32_t count;
    vkGetSwapchainImagesKHR(vk_device, vk_swap, &count, nullptr);
    std::vector<handle_t> images(count); 
    vkGetSwapchainImagesKHR(vk_device, vk_swap, &count, reinterpret_cast<VkImage*>(images.data()));
    return images;
}

void Instance::destroySwapchain(handle_t swapchain) const
{
    if (!device || !device->getHandle()) std::terminate();
    vkDestroySwapchainKHR(device->getHandle().as<VkDevice>(), static_cast<VkSwapchainKHR>(swapchain), nullptr);
}

void Instance::destroyImageView(handle_t image_view) const
{
    if (!device || !device->getHandle()) std::terminate();
    vkDestroyImageView(device->getHandle().as<VkDevice>(), static_cast<VkImageView>(image_view), nullptr);
}

handle_t Instance::createCommandPool() const
{
    if (!device || !device->getHandle()) std::terminate();

    VkCommandPoolCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = device->getGraphicsQueue().index
    };

    VkCommandPool pool;
    const auto err = vkCreateCommandPool(device->getHandle().as<VkDevice>(), &create_info, nullptr, &pool);
    if (err != VK_SUCCESS) std::terminate();

    return static_cast<handle_t>(pool);
}

void Instance::destroyCommandPool(handle_t pool) const
{
    if (!device || !device->getHandle()) std::terminate();
    vkDestroyCommandPool(device->getHandle().as<VkDevice>(), static_cast<VkCommandPool>(pool), nullptr);
}

handle_t Instance::createCommandBuffer(handle_t command_pool) const
{
    if (!device || !device->getHandle()) std::terminate();

    VkCommandBufferAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = nullptr,
        .commandPool = static_cast<VkCommandPool>(command_pool),
        .commandBufferCount = 1,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY
    };

    VkCommandBuffer buff;
    const auto err = vkAllocateCommandBuffers(device->getHandle().as<VkDevice>(), &alloc_info, &buff);
    if (err != VK_SUCCESS) std::terminate();

    return static_cast<handle_t>(buff);
}

handle_t Instance::createSemaphore() const
{
    if (!device || !device->getHandle()) std::terminate();

    VkSemaphoreCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0
    };

    VkSemaphore s;
    const auto err = vkCreateSemaphore(device->getHandle().as<VkDevice>(), &create_info, nullptr, &s);
    if (err != VK_SUCCESS) std::terminate();
    return static_cast<handle_t>(s);
}

void Instance::destroySemaphore(handle_t semaphore) const
{
    if (!device || !device->getHandle()) std::terminate();
    vkDestroySemaphore(device->getHandle().as<VkDevice>(), static_cast<VkSemaphore>(semaphore), nullptr);
}

handle_t Instance::createFence(bool signaled) const
{
    if (!device || !device->getHandle()) std::terminate();

    VkFenceCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .pNext = nullptr,
        .flags = static_cast<VkFenceCreateFlags>( signaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0 )
    };

    VkFence fence;
    const auto err = vkCreateFence(device->getHandle().as<VkDevice>(), &create_info, nullptr, &fence);
    if (err != VK_SUCCESS) std::terminate();
    return static_cast<handle_t>(fence);
}

void Instance::destroyFence(handle_t fence) const
{
    if (!device || !device->getHandle()) std::terminate();
    vkDestroyFence(device->getHandle().as<VkDevice>(), static_cast<VkFence>(fence), nullptr);
}

Instance::Instance() : handle(nullptr)
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
    handle = instance;

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

    device = std::make_unique<Device>(handle, physical_devices[0]);
}

Instance::~Instance()
{
    if (handle)
    {
        device.reset();
        vkDestroyInstance(handle.as<VkInstance>(), nullptr);
        handle = nullptr;
    }
    std::cout << "Destroy\n";
}

}