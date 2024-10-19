#pragma once

namespace mn::Graphics
{
    struct Keyboard
    {
        enum Key
        {
            W, A, S, D, Q, Space, LShift, Escape, LControl
        };

        static bool keyDown(Key key);
    };
}