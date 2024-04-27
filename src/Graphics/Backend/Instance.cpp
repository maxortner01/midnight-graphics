#include <Graphics/Backend/Instance.hpp>
#include <Graphics/Window.hpp>

#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include <vk_mem_alloc.h>

namespace mn::Graphics::Backend
{

using handle_t = mn::handle_t;

handle_t Instance::createSurface(Handle<Window> window) const
{
    MIDNIGHT_ASSERT(handle, "Invalid instance");

    VkSurfaceKHR surface;
    const auto err = SDL_Vulkan_CreateSurface(
        window.as<SDL_Window*>(),
        handle.as<VkInstance>(),
        nullptr,
        &surface
    );

    MIDNIGHT_ASSERT(err, "Error creating surface: " << SDL_GetError());

    return static_cast<handle_t>(surface);
}

void Instance::destroySurface(handle_t surface) const
{
    MIDNIGHT_ASSERT(handle, "Invalid instance");
    vkDestroySurfaceKHR(handle.as<VkInstance>(), static_cast<VkSurfaceKHR>(surface), nullptr);
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
    MIDNIGHT_ASSERT(err == VK_SUCCESS, "Error creating instance (" << err << ")");

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

    MIDNIGHT_ASSERT(physical_devices.size(), "No physical devices found");

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

    VmaAllocatorCreateInfo alloc_create_info = {
        .instance = instance,
        .device = device->getHandle().as<VkDevice>(),
        .physicalDevice = physical_devices[0]
    };

    const auto alloc = [&]()
    {
        VmaAllocator alloc;
        const auto err = vmaCreateAllocator(&alloc_create_info, &alloc);
        MIDNIGHT_ASSERT(err == VK_SUCCESS, "Error creating allocator: " << err);
        return alloc;
    }();
    
    allocator = alloc;
}

Instance::~Instance()
{
    if (handle)
    {
        vmaDestroyAllocator(static_cast<VmaAllocator>(allocator));
        allocator = nullptr;
        device.reset();
        vkDestroyInstance(handle.as<VkInstance>(), nullptr);
        handle = nullptr;
    }
    std::cout << "Destroy\n";
}

}