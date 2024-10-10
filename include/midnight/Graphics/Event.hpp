#pragma once

#include <variant>

namespace mn::Graphics
{
    struct Event
    {
        enum class ButtonType { Press, Release };

        struct None { };
        struct Quit { };
        struct Key
        {
            char key;
            ButtonType type;
        };

        struct MouseMove
        {
            Math::Vec2f delta;
        };

        struct MouseScroll
        {
            float delta;
        };

        struct WindowSize
        {
            uint32_t new_width, new_height;
        };

        std::variant<None, Quit, Key, WindowSize, MouseMove, MouseScroll> event;
    };
}