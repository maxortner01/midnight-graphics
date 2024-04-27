#pragma once

#include <Def.hpp>

#include <Graphics/Backend/Command.hpp>
#include <Graphics/Backend/Sync.hpp>

namespace mn::Graphics
{
    struct FrameData
    {
        std::unique_ptr<Backend::CommandBuffer> command_buffer;
        std::unique_ptr<Backend::CommandPool>   command_pool;
        std::unique_ptr<Backend::Semaphore> swapchain_sem, render_sem;
        std::unique_ptr<Backend::Fence> render_fence;

        void create();
        void destroy();
    };

    struct Window;

    struct RenderFrame
    {
        friend struct Window;

        const uint32_t image_index;
        const mn::handle_t image;

        void clear(std::tuple<float, float, float> color) const;

    private:
        RenderFrame(uint32_t i, mn::handle_t im) : image_index(i), image(im) { }

        std::shared_ptr<FrameData> frame_data;
    };

    struct Window
    {
        Window(const std::string& config_file = "");

        static Window fromLuaScript(const std::string& config_file);

        Window(const Window&) = delete;
        Window(Window&&);

        ~Window();

        void close();
        
        RenderFrame startFrame() const;
        void testDisplay(RenderFrame& rf) const;
        void endFrame(RenderFrame& rf) const;

        bool shouldClose() const { return _close; }

    private:
        uint32_t next_image_index() const;

        bool _close;
        Handle<Window> handle;
        mn::handle_t surface, swapchain;
        std::vector<handle_t> images;
        std::shared_ptr<FrameData> frame_data;
    };
}