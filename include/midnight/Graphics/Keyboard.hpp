#pragma once

namespace mn::Graphics
{
    struct Keyboard
    {
        enum Key
        {
            W, A, S, D, Q, Space, LShift, Escape
        };

        static bool keyDown(Key key);
    };
}