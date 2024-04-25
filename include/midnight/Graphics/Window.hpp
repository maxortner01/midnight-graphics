#pragma once

#include <Def.hpp>

#include <Graphics/Backend/Command.hpp>
#include <Graphics/Backend/Sync.hpp>

namespace mn::Graphics
{
    struct Window
    {
        Window(const std::string& config_file = "");

        static Window fromLuaScript(const std::string& config_file);

        Window(const Window&) = delete;
        Window(Window&&);

        ~Window();

        void close();
        void startFrame() const;
        bool shouldClose() const { return _close; }

        void update() const;

    private:
        struct FrameData
        {
            std::unique_ptr<Backend::CommandBuffer> command_buffer;
            std::unique_ptr<Backend::CommandPool>   command_pool;
            std::unique_ptr<Backend::Semaphore> swapchain_sem, render_sem;
            std::unique_ptr<Backend::Fence> render_fence;

            void create();
            void destroy();
        } frame_data;

        bool _close;
        Handle<Window> handle;
        mn::handle_t surface, swapchain;
        std::vector<handle_t> image_views;
    };
}