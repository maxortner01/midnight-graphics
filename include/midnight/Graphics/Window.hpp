#pragma once

#include <Def.hpp>
#include <Math.hpp>

#include <Graphics/Backend/Command.hpp>
#include <Graphics/Backend/Sync.hpp>

#include "RenderFrame.hpp"

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

    struct Window
    {
        Window(const std::string& config_file = "");

        static Window fromLuaScript(const std::string& config_file);

        Window(const Window&) = delete;
        Window(Window&&);

        ~Window();

        void close();

        auto size() const { return _size; }
        
        RenderFrame startFrame() const;
        void endFrame(RenderFrame& rf) const;
        void runFrame(const std::function<void(RenderFrame& rf)>& func) const;

        void setTitle(const std::string& title) const;
        bool shouldClose() const { return _close; }
        void finishWork() const;

    private:
        uint32_t next_image_index() const;

        bool _close;
        Handle<Window> handle;
        mn::handle_t surface, swapchain;
        std::vector<std::shared_ptr<Image>> images;
        std::shared_ptr<FrameData> frame_data;
        Math::Vec2u _size;
    };
}