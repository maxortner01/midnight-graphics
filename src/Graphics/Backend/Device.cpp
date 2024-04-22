#include <Graphics/Backend/Device.hpp>
#include <Graphics/Backend/Instance.hpp>

#include <Graphics/Window.hpp>

#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

namespace mn::Graphics::Backend
{

Device::Device(Handle<Instance> _instance, handle_t p_device) :
    physical_device(p_device)
{
    const auto instance = _instance.as<VkInstance>();
    MIDNIGHT_ASSERT(instance, "Device requires a valid instance");

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
    }(static_cast<VkPhysicalDevice>(p_device));

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

            MIDNIGHT_ASSERT(found, "Extension (" << required << ") not supported by this physical device");

            enabledExtensions.push_back(required);
        }

        return enabledExtensions;
    }(static_cast<VkPhysicalDevice>(p_device));

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
    const auto err = vkCreateDevice(static_cast<VkPhysicalDevice>(p_device), &create_info, nullptr, &_device);
    MIDNIGHT_ASSERT(err == VK_SUCCESS, "Error creating device (" << err << ")");

    std::cout << "Successfully created device\n";
    handle = _device;

    VkQueue _gq;
    vkGetDeviceQueue(_device, graphics_index, 0, &_gq);
    graphics = Queue {
        .handle = _gq,
        .index  = graphics_index
    };
}

Device::~Device()
{
    if (handle)
    {
        vkDestroyDevice(handle.as<VkDevice>(), nullptr);
        handle = nullptr;
    }
}

std::pair<handle_t, std::vector<handle_t>> Device::createSwapchain(Handle<Window> window, handle_t surface) const
{
    MIDNIGHT_ASSERT(handle, "Device invalid");
    auto* win = window.as<SDL_Window*>();
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

    MIDNIGHT_ASSERT(surfaceFormats[0].format == VK_FORMAT_B8G8R8A8_UNORM, "surfaceFormats[0].format != VK_FORMAT_B8G8R8A8_UNORM");

    const auto [w, h] = [win, c = capabilities]()
    {
        int w, h;
        SDL_GetWindowSize(win, &w, &h);
        MIDNIGHT_ASSERT(w >= 0 && h >= 0, "Window size invalid");
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
        .pQueueFamilyIndices = &graphics.index,
        .preTransform = capabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = VK_PRESENT_MODE_FIFO_KHR,
        .clipped = VK_TRUE
    };

    VkSwapchainKHR swapchain;
    const auto err = vkCreateSwapchainKHR(handle.as<VkDevice>(), &create_info, nullptr, &swapchain);
    MIDNIGHT_ASSERT(err == VK_SUCCESS, "Error creating swapchain (" << err << ")");

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
        const auto err = vkCreateImageView(handle.as<VkDevice>(), &create_info, nullptr, &image_view);
        MIDNIGHT_ASSERT(err == VK_SUCCESS, "Error creating image views (" << err << ")");

        pair.second.push_back(static_cast<handle_t>(image_view));
    }

    return pair;
}

std::vector<handle_t> Device::getSwapchainImages(handle_t swapchain) const
{
    assert(sizeof(handle_t) == sizeof(VkImage));
    const auto vk_device = handle.as<VkDevice>();
    const auto vk_swap   = static_cast<VkSwapchainKHR>(swapchain);
    uint32_t count;
    vkGetSwapchainImagesKHR(vk_device, vk_swap, &count, nullptr);
    std::vector<handle_t> images(count); 
    vkGetSwapchainImagesKHR(vk_device, vk_swap, &count, reinterpret_cast<VkImage*>(images.data()));
    return images;
}

void Device::destroySwapchain(handle_t swapchain) const
{
    MIDNIGHT_ASSERT(handle, "Invalid device");
    vkDestroySwapchainKHR(handle.as<VkDevice>(), static_cast<VkSwapchainKHR>(swapchain), nullptr);
}

void Device::destroyImageView(handle_t image_view) const
{
    MIDNIGHT_ASSERT(handle, "Invalid device");
    vkDestroyImageView(handle.as<VkDevice>(), static_cast<VkImageView>(image_view), nullptr);
}

handle_t Device::createCommandPool() const
{
    MIDNIGHT_ASSERT(handle, "Invalid device");

    VkCommandPoolCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = graphics.index
    };

    VkCommandPool pool;
    const auto err = vkCreateCommandPool(handle.as<VkDevice>(), &create_info, nullptr, &pool);
    MIDNIGHT_ASSERT(err == VK_SUCCESS, "Error creating command pool (" << err << ")");

    return static_cast<handle_t>(pool);
}

void Device::destroyCommandPool(handle_t pool) const
{
    MIDNIGHT_ASSERT(handle, "Invalid device");
    vkDestroyCommandPool(handle.as<VkDevice>(), static_cast<VkCommandPool>(pool), nullptr);
}

handle_t Device::createCommandBuffer(handle_t command_pool) const
{
    MIDNIGHT_ASSERT(handle, "Invalid device");

    VkCommandBufferAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = nullptr,
        .commandPool = static_cast<VkCommandPool>(command_pool),
        .commandBufferCount = 1,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY
    };

    VkCommandBuffer buff;
    const auto err = vkAllocateCommandBuffers(handle.as<VkDevice>(), &alloc_info, &buff);
    MIDNIGHT_ASSERT(err == VK_SUCCESS, "Error allocating command buffers (" << err << ")");

    return static_cast<handle_t>(buff);
}

handle_t Device::createSemaphore() const
{
    MIDNIGHT_ASSERT(handle, "Invalid device");

    VkSemaphoreCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0
    };

    VkSemaphore s;
    const auto err = vkCreateSemaphore(handle.as<VkDevice>(), &create_info, nullptr, &s);
    MIDNIGHT_ASSERT(err == VK_SUCCESS, "Error creating semaphore (" << err << ")");
    return static_cast<handle_t>(s);
}

void Device::destroySemaphore(handle_t semaphore) const
{
    MIDNIGHT_ASSERT(handle, "Invalid device");
    vkDestroySemaphore(handle.as<VkDevice>(), static_cast<VkSemaphore>(semaphore), nullptr);
}

handle_t Device::createFence(bool signaled) const
{
    MIDNIGHT_ASSERT(handle, "Invalid device");

    VkFenceCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .pNext = nullptr,
        .flags = static_cast<VkFenceCreateFlags>( signaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0 )
    };

    VkFence fence;
    const auto err = vkCreateFence(handle.as<VkDevice>(), &create_info, nullptr, &fence);
    MIDNIGHT_ASSERT(err == VK_SUCCESS, "Error creating fence (" << err << ")");
    return static_cast<handle_t>(fence);
}

void Device::destroyFence(handle_t fence) const
{
    MIDNIGHT_ASSERT(handle, "Invalid device");
    vkDestroyFence(handle.as<VkDevice>(), static_cast<VkFence>(fence), nullptr);
}

}
