#include <Graphics/Window.hpp>
#include <Graphics/Backend/Instance.hpp>

#include <SL/Lua.hpp>
#include <SDL3/SDL.h>

#ifndef SOURCE_DIR
#define SOURCE_DIR "."
#endif

namespace mn::Graphics
{
using handle_t = mn::handle_t;

Window::Window(const std::string& config_file)
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

    auto instance = mn::Graphics::Backend::Instance::get();
    surface   = instance->createSurface(handle);
    std::tie(swapchain, image_views) = instance->createSwapchain(handle, surface);

    frame_data.command_pool   = instance->createCommandPool();
    frame_data.command_buffer = instance->createCommandBuffer(frame_data.command_pool);

    // create semaphores
    frame_data.render_sem = instance->createSemaphore();
    frame_data.swapchain_sem = instance->createSemaphore();
    frame_data.render_fence = instance->createFence();

    _close = false;
}

void Window::close()
{
    _close = true;
}

void Window::startFrame() const
{
    /*
    auto instance = Instance::get();
    instance->waitForFence(frame_data.render_fence);
    instance->resetFence(frame_data.render_fence);

    const auto index = instance->getNextImage(swapchain, frame_data.swapchain_sem);*/

    // next we will begin command buffer and such, so we should break this file out and start RAII objects like frame_data and CommandPools and such 
    // with persistant state, that way we don't have to keep going through Instance::get()
    // Also, need to determine how far to go with RAII, maybe just stick to the higher level objects for now and maybe borrow the dequee idea from vkguide 
}

Window::~Window()
{
    if (handle)
    {
        {
            auto instance = mn::Graphics::Backend::Instance::get();
            instance->destroySemaphore(frame_data.render_sem);
            instance->destroySemaphore(frame_data.swapchain_sem);
            instance->destroyFence(frame_data.render_fence);

            instance->destroyCommandPool(frame_data.command_pool);

            for (const auto& iv : image_views)
                instance->destroyImageView(iv);   

            instance->destroySwapchain(swapchain);
            instance->destroySurface(surface);
        }
        mn::Graphics::Backend::Instance::destroy();
        SDL_DestroyWindow(static_cast<SDL_Window*>(handle));
        SDL_Quit();
        handle = nullptr;
    }
}

void Window::update() const
{
    SDL_UpdateWindowSurface(static_cast<SDL_Window*>(handle));
}

}