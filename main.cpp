#include <iostream>

#include <exception>
#include <cassert>
#include <SL/Lua.hpp>
#include <SDL3/SDL.h>

#ifndef SOURCE_DIR
#define SOURCE_DIR "."
#endif

using handle_t = void*;

template<typename T>
struct Singleton
{
    friend T;

    inline static std::shared_ptr<T> get()
    {
        if (!_instance) _instance = 
            std::shared_ptr<T>(
                new T(),
                [](void* ptr) { delete reinterpret_cast<T*>(ptr); }
            );
        return _instance;
    }

    inline static void destroy()
    {
        _instance.reset();
    }

private:
    inline static std::shared_ptr<T> _instance;
};

struct Instance : Singleton<Instance>
{
    friend class Singleton<Instance>;

protected:
    Instance()
    {
        std::cout << "Create\n";
    }

    ~Instance()
    {
        std::cout << "Destroy\n";
    }
};

struct Window
{
    Window(const std::string& config_file = "")
    {
        if (SDL_WasInit(SDL_INIT_VIDEO)) std::terminate();

        // Initialize SDL
        if (SDL_Init(SDL_INIT_VIDEO) != 0)
        {
            std::cout << "Error initializing video\n";
            std::terminate();
        }

        // Read in the window information from the file
        const auto [title, width, height] = (config_file.length() ? [](const std::string& file)
        {
            SL::Runtime runtime(SOURCE_DIR "/" + file);
            const auto res = runtime.getGlobal<SL::Table>("WindowOptions");
            assert(res);

            const auto string = res->get<SL::String>("title");
            const auto w = res->get<SL::Table>("size").get<SL::Number>("w");
            const auto h = res->get<SL::Table>("size").get<SL::Number>("h");
            return std::tuple(string, w, h);
        }(config_file) : std::tuple("window", 1280, 720) );

        // Create the window
        handle = static_cast<handle_t>(SDL_CreateWindow(title.c_str(), width, height, 0));
        if (!handle)
        {
            std::cout << "Error initializing window\n";
            std::terminate();
        }

        auto instance = Instance::get();

        _close = false;
    }

    void close()
    {
        _close = true;
    }

    bool shouldClose() const { return _close; }

    ~Window()
    {
        if (handle)
        {
            Instance::destroy();
            SDL_DestroyWindow(static_cast<SDL_Window*>(handle));
            SDL_Quit();
            handle = nullptr;
        }
    }

    void update()
    {
        SDL_UpdateWindowSurface(static_cast<SDL_Window*>(handle));
    }

private:
    bool _close;
    handle_t handle;
};

int main()
{
    Window window("window.lua");

    // Main loop
    while (!window.shouldClose())
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED)
                window.close();
        }

        window.update();
    }
}