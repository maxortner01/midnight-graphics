#pragma once

#include <midnight.hpp>

namespace Game::Components
{
    struct Transform
    {
        mn::Math::Vec3f position, scale;
        mn::Math::Vec3<mn::Math::Angle> rotation;
    };

    static mn::Math::Mat4<float> makeModelMatrix(const Transform& t)
    {
        using namespace mn;
        return Math::scale(t.scale) * Math::translation(t.position);
    }

    struct Mesh
    {
        std::shared_ptr<mn::Graphics::Model> model;
    };

    struct Camera
    {
        mn::Math::Angle FOV;
        float near, far;
    };
}