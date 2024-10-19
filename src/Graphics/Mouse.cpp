#include <midnight/Graphics/Mouse.hpp>

#include <SDL3/SDL.h>

namespace mn::Graphics
{
    bool Mouse::leftDown()
    {
        float x, y;
        auto flags = SDL_GetMouseState(&x, &y);
        return (0b1 & flags);
    }
}