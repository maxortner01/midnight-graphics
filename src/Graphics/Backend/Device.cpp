#include <Graphics/Backend/Device.hpp>
#include <Graphics/Backend/Instance.hpp>
#include <Graphics/Backend/Sync.hpp>

#include <Graphics/Window.hpp>
#include <Graphics/Image.hpp>

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

namespace mn::Graphics::Backend
{

Device::Device(Handle<Instance> _instance, handle_t p_device) :
    physical_device(p_device),
    imgui_pool{nullptr}
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
#ifdef __APPLE__
            "VK_KHR_portability_subset",
#endif
            VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME, 
            VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME, 
            VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME, 
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
            VK_KHR_DEPTH_STENCIL_RESOLVE_EXTENSION_NAME,
            VK_KHR_DEVICE_GROUP_EXTENSION_NAME,
            VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME,
            VK_KHR_MULTIVIEW_EXTENSION_NAME,
            VK_KHR_MAINTENANCE2_EXTENSION_NAME,
            VK_KHR_MAINTENANCE3_EXTENSION_NAME,
            VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME   
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

    VkPhysicalDeviceDescriptorIndexingFeatures indexing_features = 
    {
        .pNext = nullptr,
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES,
        .descriptorBindingSampledImageUpdateAfterBind = true,
        .descriptorBindingUpdateUnusedWhilePending = true,
        .descriptorBindingPartiallyBound = true,
        .descriptorBindingVariableDescriptorCount = true
    };

    VkPhysicalDeviceDynamicRenderingFeatures dynamic_render = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES,
        .pNext = &indexing_features,
        .dynamicRendering = VK_TRUE
    };

    VkPhysicalDeviceSynchronization2Features sync = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES,
        .pNext = &dynamic_render,
        .synchronization2 = VK_TRUE
    };

    VkPhysicalDeviceBufferDeviceAddressFeatures buffer_device = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES,
        .pNext = &sync,
        .bufferDeviceAddress = VK_TRUE,
        .bufferDeviceAddressCaptureReplay = VK_FALSE,
        .bufferDeviceAddressMultiDevice = VK_FALSE,
    };

    VkPhysicalDeviceFeatures features = {
        .fillModeNonSolid = VK_TRUE,
    };

    VkDeviceCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = &buffer_device,
        .flags = 0,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queue_create,
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = nullptr,
        .enabledExtensionCount   = static_cast<uint32_t>(extensions.size()),
        .ppEnabledExtensionNames = extensions.data(),
        .pEnabledFeatures = &features,
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

    // Create Samplers
    VkSampler sample;
    VkSamplerCreateInfo sampler_create_info{};
    sampler_create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

    // Nearest
    sampler_create_info.magFilter = VK_FILTER_NEAREST;
    sampler_create_info.minFilter = VK_FILTER_NEAREST;
    MIDNIGHT_ASSERT(vkCreateSampler(_device, &sampler_create_info, nullptr, &sample) == VK_SUCCESS, "Failed to create nearest sampler");
    samplers[Sampler::Nearest] = std::make_shared<Sampler>(Sampler{ .handle = static_cast<mn::handle_t>(sample) });

    // Linear
    sampler_create_info.magFilter = VK_FILTER_LINEAR;
    sampler_create_info.minFilter = VK_FILTER_LINEAR;
    MIDNIGHT_ASSERT(vkCreateSampler(_device, &sampler_create_info, nullptr, &sample) == VK_SUCCESS, "Failed to create linear sampler");
    samplers[Sampler::Linear] = std::make_shared<Sampler>(Sampler{ .handle = static_cast<mn::handle_t>(sample) });
    std::cout << "Successfully created samplers\n";
}

Device::~Device()
{
    for (const auto& [ type, sampler ] : samplers)
        vkDestroySampler(handle.as<VkDevice>(), static_cast<VkSampler>(sampler->handle), nullptr);

    if (imgui_pool)
        vkDestroyDescriptorPool(handle.as<VkDevice>(), static_cast<VkDescriptorPool>(imgui_pool), nullptr);

    if (handle)
    {
        vkDestroyDevice(handle.as<VkDevice>(), nullptr);
        handle = nullptr;
    }
}

std::tuple<handle_t, std::vector<handle_t>, uint32_t, Math::Vec2u> 
Device::createSwapchain(Handle<Window> window, handle_t surface) const
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
        .surface = vk_surface,
        .minImageCount = capabilities.minImageCount,
        .imageFormat = surfaceFormats[0].format,
        .imageColorSpace = surfaceFormats[0].colorSpace,
        .imageExtent = VkExtent2D{ .width = w, .height = h },
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        .queueFamilyIndexCount = 1,
        .pQueueFamilyIndices = &graphics.index,
        .preTransform = capabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = VK_PRESENT_MODE_FIFO_KHR,
        .clipped = VK_TRUE,
        .oldSwapchain = nullptr
    };

    VkSwapchainKHR swapchain;
    const auto err = vkCreateSwapchainKHR(handle.as<VkDevice>(), &create_info, nullptr, &swapchain);
    MIDNIGHT_ASSERT(err == VK_SUCCESS, "Error creating swapchain (" << err << ")");

    return std::tuple(
        static_cast<handle_t>(swapchain), 
        getSwapchainImages(swapchain), 
        static_cast<uint32_t>(surfaceFormats[0].format), 
        Math::Vec2u{ w, h });
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

std::pair<Handle<Image>, mn::handle_t> Device::createImage(const Math::Vec2u& size, uint32_t format, bool depth) const
{
    VkImageCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = static_cast<VkFormat>(format),
        .extent = { .width = Math::x(size), .height = Math::y(size), .depth = 1,  },
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | static_cast<VkImageUsageFlags>(depth ? (VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) : VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT),
    };

    VmaAllocationCreateInfo alloc_create_info = {
        .usage = VMA_MEMORY_USAGE_GPU_ONLY,
        .requiredFlags = static_cast<VkMemoryPropertyFlags>(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
    }; 

    const auto allocator = static_cast<VmaAllocator>(Instance::get()->getAllocator());

    VkImage _image;
    VmaAllocation _alloc;
    VmaAllocationInfo _alloc_info;
    const auto err = vmaCreateImage(allocator, &create_info, &alloc_create_info, &_image, &_alloc, &_alloc_info);
    MIDNIGHT_ASSERT(err == VK_SUCCESS, "Error creating image: " << err);
    return std::pair(Handle<Image>(_image), static_cast<mn::handle_t>(_alloc));
}

void Device::destroyImage(Handle<Image> image, mn::handle_t alloc) const
{
    const auto allocator = static_cast<VmaAllocator>(Instance::get()->getAllocator());
    vmaDestroyImage(allocator, image.as<VkImage>(), static_cast<VmaAllocation>(alloc));
}

mn::handle_t Device::createImageView(Handle<Image> image, uint32_t format, bool depth) const
{
    VkImageViewCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .image = image.as<VkImage>(),
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = static_cast<VkFormat>(format),
        .components = {
            .r = VK_COMPONENT_SWIZZLE_IDENTITY,
            .g = VK_COMPONENT_SWIZZLE_IDENTITY,
            .b = VK_COMPONENT_SWIZZLE_IDENTITY,
            .a = VK_COMPONENT_SWIZZLE_IDENTITY
        },
        .subresourceRange = {
            .aspectMask = static_cast<VkImageAspectFlags>(depth ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT),
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };

    VkImageView iv;
    const auto err = vkCreateImageView(handle.as<VkDevice>(), &create_info, nullptr, &iv);
    MIDNIGHT_ASSERT(err == VK_SUCCESS, "Error creating image view: " << err);
    return iv;
}

void Device::destroyImageView(handle_t image_view) const
{
    MIDNIGHT_ASSERT(handle, "Invalid device");
    vkDestroyImageView(handle.as<VkDevice>(), static_cast<VkImageView>(image_view), nullptr);
}

Handle<CommandPool> Device::createCommandPool() const
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

    return pool;
}

void Device::destroyCommandPool(Handle<CommandPool> pool) const
{
    MIDNIGHT_ASSERT(handle, "Invalid device");
    vkDestroyCommandPool(handle.as<VkDevice>(), pool.as<VkCommandPool>(), nullptr);
}

void Device::immediateSubmit(std::function<void(Backend::CommandBuffer&)> func) const
{
    Backend::Fence f;
    f.reset();
    Backend::CommandPool pool;

    auto cmd = pool.allocateBuffer();
    cmd->begin();
    func(*cmd);
    cmd->end();

    const auto buffer = cmd->getHandle().as<VkCommandBuffer>();
    VkSubmitInfo submit{};
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.pNext = nullptr;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &buffer;

    const auto err = vkQueueSubmit(
        static_cast<VkQueue>(graphics.handle),
        1, 
        &submit, 
        f.getHandle().as<VkFence>()
    );
    MIDNIGHT_ASSERT(err == VK_SUCCESS, "Error immediately submitting command buffer");
    f.wait();
}

Handle<CommandBuffer> Device::createCommandBuffer(Handle<CommandPool> command_pool) const
{
    MIDNIGHT_ASSERT(handle, "Invalid device");

    VkCommandBufferAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = nullptr,
        .commandPool = static_cast<VkCommandPool>(command_pool),
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1
    };

    VkCommandBuffer buff;
    const auto err = vkAllocateCommandBuffers(handle.as<VkDevice>(), &alloc_info, &buff);
    MIDNIGHT_ASSERT(err == VK_SUCCESS, "Error allocating command buffers (" << err << ")");

    return static_cast<handle_t>(buff);
}

Handle<Semaphore> Device::createSemaphore() const
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
    return s;
}

void Device::destroySemaphore(Handle<Semaphore> semaphore) const
{
    MIDNIGHT_ASSERT(handle, "Invalid device");
    vkDestroySemaphore(handle.as<VkDevice>(), semaphore.as<VkSemaphore>(), nullptr);
}

Handle<Fence> Device::createFence(bool signaled) const
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
    return fence;
}

void Device::destroyFence(Handle<Fence> fence) const
{
    MIDNIGHT_ASSERT(handle, "Invalid device");
    vkDestroyFence(handle.as<VkDevice>(), fence.as<VkFence>(), nullptr);
}

Handle<Shader> Device::createShader(const std::vector<uint32_t>& data) const
{
    VkShaderModuleCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .codeSize = data.size() * sizeof(uint32_t),
        .pCode = &data[0]
    };

    VkShaderModule shader_module;
    const auto err = vkCreateShaderModule(
        handle.as<VkDevice>(),
        &create_info,
        nullptr,
        &shader_module
    );

    MIDNIGHT_ASSERT(err == VK_SUCCESS, "Error creating shader module");
    return Handle<Shader>(shader_module);
}

void Device::destroyShader(Handle<Shader> shader) const
{
    MIDNIGHT_ASSERT(handle, "Invalid device");
    vkDestroyShaderModule(handle.as<VkDevice>(), shader.as<VkShaderModule>(), nullptr);
}

std::shared_ptr<Sampler> Device::getSampler(Sampler::Type type)
{
    return samplers[type];
}

void Device::waitForIdle() const
{
    vkDeviceWaitIdle(handle.as<VkDevice>());
}

mn::handle_t Device::getImGuiPool()
{
    if (imgui_pool) return imgui_pool;

    // Create imgui pool
    // https://vkguide.dev/docs/extra-chapter/implementing_imgui/
    VkDescriptorPoolSize pool_sizes[] =
	{
		{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
	};

	VkDescriptorPoolCreateInfo pool_info = {};
	pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	pool_info.maxSets = 1000;
	pool_info.poolSizeCount = std::size(pool_sizes);
	pool_info.pPoolSizes = pool_sizes;

	VkDescriptorPool imguiPool;
	MIDNIGHT_ASSERT(vkCreateDescriptorPool(handle.as<VkDevice>(), &pool_info, nullptr, &imguiPool) == VK_SUCCESS, "Error creating ImGui pool");
    imgui_pool = static_cast<mn::handle_t>(imguiPool);

    return imgui_pool;
}

}
