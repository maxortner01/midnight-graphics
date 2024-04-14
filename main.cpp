#include <iostream>

#include <exception>
#include <cassert>
#include <vector>

#include <vulkan/vulkan.h>
#include <SL/Lua.hpp>
#include <SDL3/SDL.h>

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

        std::vector<const char*> enabled_extensions = { VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME };
        create_info.enabledExtensionCount   = enabled_extensions.size();
        create_info.ppEnabledExtensionNames = enabled_extensions.data();

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
            std::vector<const char*> requiredExtensions = { VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME, VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME, VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME };

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

                return enabledExtensions;
            }
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

    handle_t handle, device;
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
        handle = static_cast<handle_t>(SDL_CreateWindow(title.c_str(), width, height, 0));
        if (!handle)
        {
            std::cout << "Error initializing window\n";
            std::terminate();
        }

        auto instance = Instance::get();

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
    handle_t handle;
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