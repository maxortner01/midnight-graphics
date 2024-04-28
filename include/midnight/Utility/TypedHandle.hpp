#pragma once

#include <functional>

namespace mn::Utility
{
    template<typename Type, typename Underlying>
    struct TypedHandle
    {
        TypedHandle() : value{0}
        {   }

        TypedHandle(Underlying U) : value(U)
        {   }

        Underlying operator*() const
        {
            return this->get();
        }

        Underlying get() const
        {
            return value;
        }

        template<typename A>
        A as() const
        {
            return static_cast<A>(value);
        }

        // bug: this allows us to convert to other handles too, we need to disallow this
        template<typename A>
        operator A() const
        {
            return static_cast<A>(value);
        }

        template<typename A>
        void destroy(std::function<void(A)> d)
        {
            if (value)
            {
                d(static_cast<A>(value));
                value = 0;
            }
        }

    private:
        Underlying value;
    };
}