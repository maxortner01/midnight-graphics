#pragma once

#include <memory>

namespace mn::Utility
{
    template<typename T>
    struct Singleton
    {
        friend T;

        Singleton() = default;
        Singleton(Singleton&&) = delete;
        Singleton(const Singleton&) = delete;

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
}
