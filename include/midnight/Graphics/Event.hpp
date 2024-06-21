#pragma once

#include <variant>

namespace mn::Graphics
{
    struct Event
    {
        enum class ButtonType { Press, Release, Hold };

        struct None { };
        struct Quit { };
        struct Button
        {
            ButtonType type;
        };

        std::variant<None, Quit, Button> event;
    };
}