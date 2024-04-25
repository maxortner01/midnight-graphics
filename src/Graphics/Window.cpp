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
    MIDNIGHT_ASSERT(!SDL_WasInit(SDL_INIT_VIDEO), "SDL has already been initialized!");
    MIDNIGHT_ASSERT(!SDL_Init(SDL_INIT_VIDEO), "Error initializing video");

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
    MIDNIGHT_ASSERT(handle, "Error initializing window");

    auto instance = mn::Graphics::Backend::Instance::get();
    const auto& device = instance->getDevice();
    surface   = instance->createSurface(handle);
    std::tie(swapchain, image_views) = device->createSwapchain(handle, surface);

    frame_data.create();

    _close = false;
}

Window::Window(Window&& window) :
    _close(window._close),
    handle(window.handle),
    surface(window.surface),
    swapchain(window.swapchain)
{
    window.handle = nullptr;
    window.surface = nullptr;
    window.swapchain = nullptr;
}

Window Window::fromLuaScript(const std::string& config_file)
{
    return Window(config_file);
}

void Window::close()
{
    _close = true;
}

void Window::startFrame() const
{
    frame_data.render_fence->wait();
    frame_data.render_fence->reset();

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
            const auto& device = instance->getDevice();
            frame_data.destroy();

            for (const auto& iv : image_views)
                device->destroyImageView(iv);   

            device->destroySwapchain(swapchain);
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

void Window::FrameData::create()
{
    auto& device = Backend::Instance::get()->getDevice();
    command_pool = std::make_unique<Graphics::Backend::CommandPool>();
    command_buffer = command_pool->allocateBuffer();

    // create semaphores
    render_sem    = std::make_unique<Backend::Semaphore>();
    swapchain_sem = std::make_unique<Backend::Semaphore>();
    render_fence  = std::make_unique<Backend::Fence>();
}

void Window::FrameData::destroy()
{
    render_sem.reset();
    swapchain_sem.reset();
    render_fence.reset();
    command_pool.reset();
}

}