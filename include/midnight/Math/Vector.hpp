#pragma once

#include <Def.hpp>

#include <array>
#include <initializer_list>

namespace mn::Math
{
    template<std::size_t C, typename T>
    struct Vec
    {
        inline constexpr static std::size_t Size = C;

        T c[C];

        Vec()
        {
            std::memset(c, 0, sizeof(c));
        }

        Vec(const std::array<T, C>& vals)
        {
            std::uninitialized_copy(vals.begin(), vals.end(), &c[0]);
        }

        template<typename A>
        operator Vec<C, A>() const
        {
            Vec<C, A> r;
            for (uint32_t i = 0; i < C; i++)
                r.c[i] = static_cast<A>(c[i]);
            return r;
        }

        template<typename A>
        Vec(std::initializer_list<A> v)
        {
            MIDNIGHT_ASSERT(v.size() == C, "Size mismatch");
            for (uint32_t i = 0; i < C; i++)
                c[i] = static_cast<T>(*(v.begin() + i));
        }

        template<typename A>
        auto operator+(const Vec<C, A>& vec) const
        {
            Vec<C, T> r;
            for (uint32_t i = 0; i < C; i++)
                r.c[i] = c[i] + vec.c[i];
            return r;
        }

        template<typename A>
        auto operator-(const Vec<C, A>& vec) const
        {
            Vec<C, T> r;
            for (uint32_t i = 0; i < C; i++)
                r.c[i] = c[i] - vec.c[i];
            return r;
        }

        template<typename A>
        auto operator*(const A& sc) const
        {
            Vec<C, T> r;
            for (uint32_t i = 0; i < C; i++)
                r.c[i] = c[i] * sc;
            return r;
        }

        template<typename A>
        auto operator/(const A& sc) const
        {
            Vec<C, T> r;
            for (uint32_t i = 0; i < C; i++)
                r.c[i] = c[i] / sc;
            return r;
        }

        template<typename A>
        void operator+=(const Vec<C, A>& vec)
        {
            for (uint32_t i = 0; i < C; i++)
                c[i] += vec.c[i];
        }

        template<typename A>
        void operator-=(const Vec<C, A>& vec)
        {
            for (uint32_t i = 0; i < C; i++)
                c[i] -= vec.c[i];
        }

        template<typename A>
        void operator=(const Vec<C, A>& vec)
        {
            for (uint32_t i = 0; i < C; i++)
                c[i] = static_cast<T>(vec.c[i]);
        }

        bool operator==(const Vec<C, T>& vec) const
        {
            for (uint32_t i = 0; i < C; i++)
                if (c[i] != vec.c[i])
                    return false;
            return true;
        }
    };

    template<std::size_t C, typename T>
    static Vec<C, T> min(const Vec<C, T>& v1, const Vec<C, T>& v2)
    {
        Vec<C, T> ret;
        for (int i = 0; i < C; i++)
            ret.c[i] = std::min(v1.c[i], v2.c[i]);
        return ret;
    }

    template<std::size_t C, typename T>
    static Vec<C, T> max(const Vec<C, T>& v1, const Vec<C, T>& v2)
    {
        Vec<C, T> ret;
        for (int i = 0; i < C; i++)
            ret.c[i] = std::max(v1.c[i], v2.c[i]);
        return ret;
    }

    template<std::size_t C, typename T, 
        typename = std::enable_if_t<C >= 3>>
    static Vec<3, T> xyz(const Vec<C, T>& vec)
    {
        Vec<3, T> ret;
        for (int i = 0; i < 3; i++)
            ret.c[i] = vec.c[i];
        return ret;
    }

    template<std::size_t C, typename T>
    static T& x(Vec<C, T>& vec)
    {
        static_assert(C >= 1);
        return vec.c[0];
    }

    template<std::size_t C, typename T>
    static T& y(Vec<C, T>& vec)
    {
        static_assert(C >= 2);
        return vec.c[1];
    }

    template<std::size_t C, typename T>
    static T& z(Vec<C, T>& vec)
    {
        static_assert(C >= 3);
        return vec.c[2];
    }

    template<std::size_t C, typename T>
    static const T& x(const Vec<C, T>& vec)
    {
        static_assert(C >= 1);
        return vec.c[0];
    }

    template<std::size_t C, typename T>
    static const T& y(const Vec<C, T>& vec)
    {
        static_assert(C >= 2);
        return vec.c[1];
    }

    template<std::size_t C, typename T>
    static const T& z(const Vec<C, T>& vec)
    {
        static_assert(C >= 3);
        return vec.c[2];
    }

    template<std::size_t C, typename T>
    static T& w(Vec<C, T>& vec)
    {
        static_assert(C >= 4);
        return vec.c[3];
    }

    template<std::size_t C, typename T>
    static const T& w(const Vec<C, T>& vec)
    {
        static_assert(C >= 4);
        return vec.c[3];
    }

    template<std::size_t C, typename T1, typename T2>
    static T1 inner(const Vec<C, T1>& vec1, const Vec<C, T2>& vec2)
    {
        T1 t{0};
        for (uint32_t i = 0; i < C; i++)
            t += vec1.c[i] * vec2.c[i];
        return t;
    }

    template<std::size_t C, typename T>
    static double length(const Vec<C, T>& vec)
    {
        return sqrt( inner(vec, vec) );
    }

    template<std::size_t C, typename T>
    static Vec<C, T> normalized(const Vec<C, T>& v)
    {
        return v / length(v);
    }

    template<std::size_t C, typename T>
    static Vec<C, T> reflect(const Vec<C, T>& incident, const Vec<C, T>& normal)
    {
        return incident - normal * static_cast<T>(2.0) * inner(incident, normal);
    }

    // Technically the outer product is defined for all dimensions and the cross-product
    // is restricted to only 3 dimensions... but I'll leave it for posterity's sake
    template<typename T>
    static Vec<3, T> outer(const Vec<3, T>& vec1, const Vec<3, T>& vec2)
    {
        return Vec<3, T>({
            y(vec1) * z(vec2) - z(vec1) * y(vec2),
            z(vec1) * x(vec2) - x(vec1) * z(vec2),
            x(vec1) * y(vec2) - y(vec1) * x(vec2)
        });
    }

    template<typename T>
    using Vec2 = Vec<2, T>;
    using Vec2f = Vec2<r32>;
    using Vec2u = Vec2<u32>;
    using Vec2i = Vec2<i32>;

    template<typename T>
    using Vec3 = Vec<3, T>;
    using Vec3f = Vec3<r32>;
    using Vec3u = Vec3<u32>;
    using Vec3i = Vec3<i32>;

    template<typename T>
    using Vec4 = Vec<4, T>;
    using Vec4f = Vec4<r32>;
}
