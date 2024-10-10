#include <Graphics/Window.hpp>
#include <Graphics/RenderFrame.hpp>

#include <Graphics/Backend/Instance.hpp>

#include <imgui.h>
#include <backends/imgui_impl_vulkan.h>
#include <backends/imgui_impl_sdl3.h>

#include <SL/Lua.hpp>
#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>

#ifndef SOURCE_DIR
#define SOURCE_DIR "."
#endif

namespace mn::Graphics
{
using handle_t = mn::handle_t;

void FrameData::create()
{
    auto& device = Backend::Instance::get()->getDevice();
    command_pool = std::make_unique<Graphics::Backend::CommandPool>();
    command_buffer = command_pool->allocateBuffer();

    // create semaphores
    render_sem    = std::make_unique<Backend::Semaphore>();
    swapchain_sem = std::make_unique<Backend::Semaphore>();
    render_fence  = std::make_unique<Backend::Fence>();
}

void FrameData::destroy()
{
    render_sem.reset();
    swapchain_sem.reset();
    render_fence.reset();
    command_pool.reset();
}

Window::Window(const Math::Vec2u& size, const std::string& name)
{
    _open(size, name);

    // IF IMGUI
    ImGui::CreateContext();
    ImGui_ImplSDL3_InitForVulkan(handle.as<SDL_Window*>());
    const auto& instance = Backend::Instance::get();

    VkPipelineRenderingCreateInfoKHR c_info{};
    
    const auto format = VkFormat::VK_FORMAT_B8G8R8A8_UNORM;
    c_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
    c_info.colorAttachmentCount = 1;
    c_info.pColorAttachmentFormats = &format;
    c_info.depthAttachmentFormat = VkFormat::VK_FORMAT_D32_SFLOAT_S8_UINT;

    ImGui_ImplVulkan_InitInfo init_info = {};
	init_info.Instance = instance->getHandle().as<VkInstance>();
	init_info.PhysicalDevice = static_cast<VkPhysicalDevice>( instance->getDevice()->getPhysicalDevice() );
	init_info.Device = instance->getDevice()->getHandle().as<VkDevice>();
	init_info.Queue = static_cast<VkQueue>( instance->getDevice()->getGraphicsQueue().handle );
	init_info.DescriptorPool = static_cast<VkDescriptorPool>( instance->getDevice()->getImGuiPool() );
    init_info.PipelineRenderingCreateInfo = c_info;
    init_info.UseDynamicRendering = true;
	init_info.MinImageCount = 3;
	init_info.ImageCount = 3;
	init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

    // Actually create the imgui image
    /*
    imgui_surface = std::make_shared<Image>(
        ImageFactory()
            .addAttachment<Image::Color>(Image::B8G8R8A8_UNORM, size)
            .build()
    );*/

    ImGui_ImplVulkan_Init(&init_info);
    ImGui_ImplVulkan_CreateFontsTexture();
}

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

    _open({ width, height }, title);   
}

void Window::construct_swapchain()
{
    auto& device = Backend::Instance::get()->getDevice();
    const auto [ s, _images, format, size ] = device->createSwapchain(handle, surface);
    for (const auto& image : _images)
    {
        images.emplace_back(std::make_shared<Image>(
            ImageFactory()
                .addImage<Image::Color>(image, format, size)
                .addAttachment<Image::DepthStencil>(Image::DF32_SU8, size)
                .build()
        ));
    }
    swapchain = s;
}

void Window::_open(const Math::Vec2u& req_size, const std::string& name)
{
    // Create the window
    handle = static_cast<handle_t>(SDL_CreateWindow(name.c_str(), Math::x(req_size), Math::y(req_size), SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE));
    MIDNIGHT_ASSERT(handle, "Error initializing window");
    
    _size = req_size;

    auto instance = mn::Graphics::Backend::Instance::get();
    const auto& device = instance->getDevice();
    surface   = instance->createSurface(handle);

    construct_swapchain();

    uint32_t concurrent_frames = 2;
    for (uint32_t i = 0; i < concurrent_frames; i++)
    {
        auto fd = std::make_shared<FrameData>();
        fd->create();
        frame_data.push_back(fd);
    }

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

char key(SDL_Keycode code)
{
    switch (code)
    {
    case SDLK_W: return 'w';
    case SDLK_S: return 's';
    case SDLK_A: return 'a';
    case SDLK_D: return 'd';
    default: return ' ';
    }
}

bool Window::pollEvent(Event& event) const
{
    SDL_Event e;
    const auto res = SDL_PollEvent(&e);
    event.event = Event::None{};

    // We need to rebuild the swapchain 
    auto io = ImGui::GetIO();

    //if (!ImGui_ImplSDL3_ProcessEvent(&e))
    ImGui_ImplSDL3_ProcessEvent(&e);
    switch (e.type)
    {
    case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
        event.event = Event::Quit{};
        break;
    case SDL_EVENT_WINDOW_RESIZED:
    {
        const uint32_t new_width  = e.window.data1;
        const uint32_t new_height = e.window.data2;

        auto& device = Backend::Instance::get()->getDevice();
        device->waitForIdle();

        device->destroySwapchain(swapchain);
        // Gross
        auto* abused_window = const_cast<Window*>(this);
        abused_window->images.clear();
        abused_window->construct_swapchain();

        event.event = Event::WindowSize{ .new_width = new_width, .new_height = new_height };
        break;
    }
    case SDL_EVENT_MOUSE_MOTION:
        if (!io.WantCaptureMouse)
            event.event = Event::MouseMove{ .delta = { e.motion.xrel, e.motion.yrel } };
        break;
    case SDL_EVENT_KEY_DOWN:
        if (!io.WantCaptureKeyboard)
            event.event = Event::Key { .key = (char)e.key.key, .type = Event::ButtonType::Press };
        break;
    case SDL_EVENT_KEY_UP:
        if (!io.WantCaptureKeyboard)
            event.event = Event::Key { .key = (char)e.key.key, .type = Event::ButtonType::Release };
        break;
    default: 
        event.event = Event::None{};
        break;
    }

    return res;
}

void transition_image(VkCommandBuffer cmd, VkImage image, VkImageLayout old_layout, VkImageLayout new_layout)
{
    VkImageAspectFlags aspectMask = (new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) ? VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
    VkImageMemoryBarrier2 image_barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .pNext = nullptr,
        .srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
        .srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
        .dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT,
        .oldLayout = old_layout,
        .newLayout = new_layout,
        .image = image,
        .subresourceRange = [](VkImageAspectFlags aspectMask)
            {
                return VkImageSubresourceRange {
                    .aspectMask = aspectMask,
                    .baseMipLevel = 0,
                    .levelCount = VK_REMAINING_MIP_LEVELS,
                    .baseArrayLayer = 0,
                    .layerCount = VK_REMAINING_ARRAY_LAYERS
                };
            }(aspectMask)
    };

    VkDependencyInfo dep_info = {
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .pNext = nullptr,
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &image_barrier
    };
    
    auto& device = Backend::Instance::get()->getDevice();
    PFN_vkVoidFunction pVkCmdPipelineBarrier2KHR = vkGetDeviceProcAddr(device->getHandle().as<VkDevice>(), "vkCmdPipelineBarrier2KHR");
    ((PFN_vkCmdPipelineBarrier2KHR)(pVkCmdPipelineBarrier2KHR ))(cmd, &dep_info);
};

RenderFrame Window::startFrame() const
{
    auto next_frame = get_next_frame();
    next_frame->render_fence->wait();
    // Free resources
    next_frame->render_fence->reset();
    auto n_image = next_image_index(next_frame);

    auto& device = Backend::Instance::get()->getDevice();
    vkQueueWaitIdle(static_cast<VkQueue>(device->getGraphicsQueue().handle));
    next_frame->command_buffer->reset();
    next_frame->command_buffer->begin();

    auto _image = static_cast<VkImage>(images[n_image]->getAttachment<Image::Color>().handle);
    auto _depth_image = static_cast<VkImage>(images[n_image]->getAttachment<Image::DepthStencil>().handle);
    auto _cmd   = next_frame->command_buffer->getHandle().as<VkCommandBuffer>();
    transition_image(_cmd, _image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
    transition_image(_cmd, _depth_image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

    RenderFrame frame(n_image, images[n_image]);
    frame.frame_data = next_frame;

    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    return frame;
}

void Window::endFrame(RenderFrame& rf) const
{
    ImGui::Render();

    // Render the ImGui data onto the surface
    //rf.image_stack.push(imgui_surface);
    //rf.clear({ 0.f, 0.f, 0.f }, 0.f);
    rf.startRender();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), rf.frame_data->command_buffer->getHandle().as<VkCommandBuffer>());
    rf.endRender();
    //rf.image_stack.pop();
    
    auto _image = static_cast<VkImage>(images[rf.image_index]->getAttachment<Image::Color>().handle);
    auto _cmd   = rf.frame_data->command_buffer->getHandle().as<VkCommandBuffer>();
    transition_image(_cmd, _image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

    // Blit the imgui surface onto the main image
    // Blitting doesn't handle alpha, so here we'd actually want to draw a quad...
    // Currently don't have a nice way to do that
    //rf.blit(imgui_surface, images[rf.image_index]);

    rf.frame_data->command_buffer->end();

    //SDL_UpdateWindowSurface(static_cast<SDL_Window*>(handle));
    const auto semaphore_submit_info = [](VkPipelineStageFlags2 stageMask, VkSemaphore semaphore)
    {
        VkSemaphoreSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
        submitInfo.pNext = nullptr;
        submitInfo.semaphore = semaphore;
        submitInfo.stageMask = stageMask;
        submitInfo.deviceIndex = 0;
        submitInfo.value = 1;

        return submitInfo;
    };

    const auto command_buffer_submit_info = [](VkCommandBuffer cmd)
    {
        VkCommandBufferSubmitInfo info{};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
        info.pNext = nullptr;
        info.commandBuffer = cmd;
        info.deviceMask = 0;

        return info;
    };

    const auto submit_info = [](VkCommandBufferSubmitInfo* cmd, VkSemaphoreSubmitInfo* signalSemaphoreInfo,
        VkSemaphoreSubmitInfo* waitSemaphoreInfo)
    {
        VkSubmitInfo2 info = {};
        info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
        info.pNext = nullptr;

        info.waitSemaphoreInfoCount = waitSemaphoreInfo == nullptr ? 0 : 1;
        info.pWaitSemaphoreInfos = waitSemaphoreInfo;

        info.signalSemaphoreInfoCount = signalSemaphoreInfo == nullptr ? 0 : 1;
        info.pSignalSemaphoreInfos = signalSemaphoreInfo;

        info.commandBufferInfoCount = 1;
        info.pCommandBufferInfos = cmd;

        return info;
    };

    auto cmd_info    = command_buffer_submit_info(rf.frame_data->command_buffer->getHandle().as<VkCommandBuffer>());
    auto wait_info   = semaphore_submit_info(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, rf.frame_data->swapchain_sem->getHandle().as<VkSemaphore>());
	auto signal_info = semaphore_submit_info(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, rf.frame_data->render_sem->getHandle().as<VkSemaphore>());	
	
	const auto submit = submit_info(&cmd_info, &signal_info, &wait_info);

    auto& device = Backend::Instance::get()->getDevice();
    
	//submit command buffer to the queue and execute it.
	// _renderFence will now block until the graphic commands finish execution
    {
        // Lock the crap up here
        PFN_vkVoidFunction pvkQueueSubmit2KHR = vkGetDeviceProcAddr(device->getHandle().as<VkDevice>(), "vkQueueSubmit2KHR");
        ((PFN_vkQueueSubmit2KHR)(pvkQueueSubmit2KHR))(static_cast<VkQueue>(device->getGraphicsQueue().handle), 1, &submit, rf.frame_data->render_fence->getHandle().as<VkFence>());
    }
    const auto _swapchain = static_cast<VkSwapchainKHR>(swapchain);
    const auto _sema = static_cast<VkSemaphore>(rf.frame_data->render_sem->getHandle());
    VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.pNext = nullptr;
	presentInfo.pSwapchains = &_swapchain;
	presentInfo.swapchainCount = 1;

	presentInfo.pWaitSemaphores = &_sema;
	presentInfo.waitSemaphoreCount = 1;

	presentInfo.pImageIndices = &rf.image_index;

	const auto err = vkQueuePresentKHR(static_cast<VkQueue>(device->getGraphicsQueue().handle), &presentInfo);
}

void Window::runFrame(const std::function<void(RenderFrame& rf)>& func) const
{
    auto frame = startFrame();
    func(frame);
    endFrame(frame);
}

void Window::setTitle(const std::string& title) const
{
    SDL_SetWindowTitle(handle.as<SDL_Window*>(), title.c_str());
}

void Window::finishWork() const
{
    auto instance = mn::Graphics::Backend::Instance::get();
    const auto& device = instance->getDevice();
    vkQueueWaitIdle(static_cast<VkQueue>(device->getGraphicsQueue().handle));
}

void Window::setMousePos(Math::Vec2f position)
{
    auto flags = SDL_GetWindowFlags(handle.as<SDL_Window*>());
    if (flags & SDL_WINDOW_INPUT_FOCUS)
        SDL_WarpMouseInWindow(handle.as<SDL_Window*>(), Math::x(position), Math::y(position));
}

Window::~Window()
{
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    if (handle)
    {
	    finishWork();
        {
            auto instance = mn::Graphics::Backend::Instance::get();
            const auto& device = instance->getDevice();
            finishWork();

            images.clear();

            for (auto& f : frame_data)
                f->destroy();

            device->destroySwapchain(swapchain);
            instance->destroySurface(surface);
        }
        mn::Graphics::Backend::Instance::destroy();
        SDL_DestroyWindow(static_cast<SDL_Window*>(handle));
        SDL_Quit();
        handle = nullptr;
    }
}

uint32_t Window::next_image_index(std::shared_ptr<FrameData> fd) const
{
    auto& device = Backend::Instance::get()->getDevice();

    uint32_t index;
    const auto err = vkAcquireNextImageKHR(
        device->getHandle().as<VkDevice>(), 
        static_cast<VkSwapchainKHR>(swapchain), 
        1000000000, 
        fd->swapchain_sem->getHandle().as<VkSemaphore>(),
        VK_NULL_HANDLE,
        &index);
    MIDNIGHT_ASSERT(err == VK_SUCCESS, "Error getting next image: " << err);

    return index;
}

std::shared_ptr<FrameData> Window::get_next_frame() const
{
    static uint32_t FRAMES = 0;
    return frame_data[FRAMES++ % frame_data.size()];
}

}
