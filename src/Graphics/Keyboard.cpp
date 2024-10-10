#include <Graphics/Keyboard.hpp>

#include <SDL3/SDL.h>

namespace mn::Graphics
{
    SDL_Scancode get_key_code(Keyboard::Key key)
    {
        switch (key)
        {
        case Keyboard::W: return SDL_SCANCODE_W;
        case Keyboard::A: return SDL_SCANCODE_A;
        case Keyboard::S: return SDL_SCANCODE_S;
        case Keyboard::D: return SDL_SCANCODE_D;
        case Keyboard::Q: return SDL_SCANCODE_Q;
        case Keyboard::Space: return SDL_SCANCODE_SPACE;
        case Keyboard::LShift: return SDL_SCANCODE_LSHIFT;
        case Keyboard::Escape: return SDL_SCANCODE_ESCAPE;
        default: return SDL_SCANCODE_0;
        }
    }

    bool Keyboard::keyDown(Key key)
    {
        const bool* keys = SDL_GetKeyboardState(nullptr);
        return keys[get_key_code(key)];
    }
}