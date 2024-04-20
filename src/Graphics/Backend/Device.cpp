#include <Graphics/Backend/Device.hpp>
#include <Graphics/Backend/Instance.hpp>

#include <vulkan/vulkan.h>

namespace mn::Graphics::Backend
{

Device::Device(Handle<Instance> _instance, handle_t p_device) :
    physical_device(p_device)
{
    const auto instance = _instance.as<VkInstance>();
    if (!instance) std::terminate();

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

            if (!found)
            {
                std::cout << "Extension (" << required << ") not supported by this physical device" << std::endl;
                std::terminate();
            }

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
    if (err != VK_SUCCESS)
    {
        std::cout << "Error creating device (" << err << ")" << std::endl;
        std::terminate();
    }

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

}