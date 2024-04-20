#pragma once

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

        template<typename A>
        operator A() const
        {
            return static_cast<A>(value);
        }

    private:
        Underlying value;
    };
}