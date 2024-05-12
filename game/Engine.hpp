#pragma once

#include <midnight.hpp>

#include <flecs.h>

namespace Game
{
    struct Scene
    {
        virtual ~Scene() = default;
        
        virtual void update(double dt) { }
        virtual bool done() = 0;
        virtual void destroy() = 0;
        virtual void finish() = 0;

        flecs::world world;
    };


    class Engine : public mn::Utility::Singleton<Engine>
    {
        struct FrameInfo
        {
            mn::Math::Mat4<float> proj, view;
            mn::Math::Vec3f player_pos;
        };

        struct Constants
        {
            uint32_t index;
            mn::Graphics::Buffer::gpu_addr buffer, frame_info;
        };

        struct FundBuffers
        {
            mn::Graphics::TypeBuffer<FrameInfo> frame_info;
            mn::Graphics::Vector<mn::Math::Mat4<float>> models;
        };

    public:
        void start(const std::string& location, std::function<Scene*()> construct_scene);

    private:
        friend struct Singleton;

        Engine()
        {	}

        std::unique_ptr<Scene> scene;
    };
}
