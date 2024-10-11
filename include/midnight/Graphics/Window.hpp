#pragma once

#include <Def.hpp>
#include <Math.hpp>
#include <set>

#include <Graphics/Backend/Command.hpp>
#include <Graphics/Backend/Sync.hpp>

#include "Event.hpp"
#include "RenderFrame.hpp"

namespace mn::Graphics
{
    struct FrameData
    {
        std::unique_ptr<Backend::CommandBuffer> command_buffer;
        std::unique_ptr<Backend::CommandPool>   command_pool;
        std::unique_ptr<Backend::Semaphore> swapchain_sem, render_sem;
        std::unique_ptr<Backend::Fence> render_fence;

        // Here we can keep a std::vector<std::shared_ptr<void>> resources
        // Everytime we use something in RenderFrame, we can push it onto this resources
        // vector. Then, at the beginning of the frame when we wait on the render_fence (or during destruction
        // of this FrameData object) we can clear it. This way we ensure if the resources used on this render
        // are deleted elsewhere, they are at least still valid until the end of the render.

        std::set<std::shared_ptr<void>> resources;

        void release();

        void create();
        void destroy();
    };

    struct Window
    {
        inline static bool ImGui_Initialized = false;

        MN_SYMBOL Window(const Math::Vec2u& size, const std::string& name);
        MN_SYMBOL Window(const std::string& config_file = "");

        MN_SYMBOL static Window fromLuaScript(const std::string& config_file);

        Window(const Window&) = delete;
        Window(Window&&);

        MN_SYMBOL ~Window();

        MN_SYMBOL void close();

        MN_SYMBOL bool pollEvent(Event& event) const;

        auto size() const { return _size; }
        
        MN_SYMBOL RenderFrame startFrame() const;
        MN_SYMBOL void endFrame(RenderFrame& rf) const;
        MN_SYMBOL void runFrame(const std::function<void(RenderFrame& rf)>& func) const;

        MN_SYMBOL void setTitle(const std::string& title) const;
        bool shouldClose() const { return _close; }
        MN_SYMBOL void finishWork() const;

        MN_SYMBOL void showMouse(bool show);
        MN_SYMBOL void setMousePos(Math::Vec2f position);

        float aspectRatio() const { return (float)Math::x(_size) / (float)Math::y(_size); }

    private:
        void construct_swapchain();

        void _open(const Math::Vec2u& size, const std::string& name);

        uint32_t next_image_index(std::shared_ptr<FrameData> fd) const;
        std::shared_ptr<FrameData> get_next_frame() const;

        bool _close;
        Handle<Window> handle;
        mn::handle_t surface, swapchain;
        
        std::shared_ptr<Image> imgui_surface;
        std::vector<std::shared_ptr<Image>> images;

        std::vector<std::shared_ptr<FrameData>> frame_data;
        Math::Vec2u _size;
    };
}