#pragma once

#include <Def.hpp>

namespace mn::Graphics
{
    struct Window
    {
        Window(const std::string& config_file = "");
        ~Window();

        void close();
        void startFrame() const;
        bool shouldClose() const { return _close; }

        void update() const;

    private:
        struct FrameData
        {
            mn::handle_t command_pool, command_buffer, swapchain_sem, render_sem, render_fence;
        } frame_data;

        bool _close;
        mn::handle_t handle, surface, swapchain;
        std::vector<handle_t> image_views;
    };
}