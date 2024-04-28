#pragma once

#include <Def.hpp>

#include "Angle.hpp"

namespace mn::Math
{
    template<std::size_t R, std::size_t C, typename T>
    struct Mat
    {
        T m[R][C];

        Mat()
        {
            for (uint32_t i = 0; i < R; i++)
                std::memset(m[i], 0, sizeof(m[i]));
        }

        template<typename A>
        Mat(const Mat<R, C, A>& mat)
        {
            for (uint32_t r = 0; r < R; r++)
                for (uint32_t c = 0; c < C; c++)
                    m[r][c] = static_cast<T>(mat.m[r][c]);
        }

        template<typename A>
        auto operator+(const Mat<R, C, A>& mat) const
        {
            Mat<R, C, T> _r;
            for (uint32_t r = 0; r < R; r++)
                for (uint32_t c = 0; c < C; c++)
                    _r.m[r][c] = m[r][c] + mat.m[r][c];
            return _r;
        }

        template<typename A>
        auto operator-(const Mat<R, C, A>& mat) const
        {
            Mat<R, C, T> _r;
            for (uint32_t r = 0; r < R; r++)
                for (uint32_t c = 0; c < C; c++)
                    _r.m[r][c] = m[r][c] - mat.m[r][c];
            return _r;
        }

        template<std::size_t C2, typename A>
        auto operator*(const Mat<C, C2, A>& mat) const
        {
            Mat<R, C2, T> r;

            for (int i = 0; i < R; i++)
                for (int j = 0; j < C2; j++)
                    for (int k = 0; k < C; k++)
                        r.m[i][j] += m[i][k] * mat.m[k][j];

            return r;
        }

        template<typename A>
        auto operator*(const A& sc) const
        {
            Mat<R, C, T> _r;
            for (uint32_t r = 0; r < R; r++)
                for (uint32_t c = 0; c < C; c++)
                    _r.m[r][c] = m[r][c] * sc;
            return _r;
        }
    };

    template<std::size_t S, typename T>
    static Mat<S, S, T> identity()
    {
        Mat<S, S, T> m;
        for (uint32_t i = 0; i < S; i++)
            m.m[i][i] = 1;
        return m;
    }

    template<typename T>
    static Mat<4, 4, T> rotationX(const Angle& angle)
    {
        auto m = identity<4, T>();

        const auto _cos = static_cast<T>(cos(angle.asRadians()));
        const auto _sin = static_cast<T>(sin(angle.asRadians()));

        m.m[1][1] = _cos; m.m[2][2] = _cos;
        m.m[1][2] = static_cast<T>(-1.0 * _sin); m.m[2][1] = _sin;

        return m;
    }

    template<typename T>
    static Mat<4, 4, T> rotationY(const Angle& angle)
    {        
        auto m = identity<4, T>();

        const auto _cos = static_cast<T>(cos(angle.asRadians()));
        const auto _sin = static_cast<T>(sin(angle.asRadians()));

        m.m[0][0] = _cos; m.m[2][2] = _cos;
        m.m[2][0] = static_cast<T>(-1.0 * _sin); m.m[0][2] = _sin;

        return m;
    }


    template<typename T>
    static Mat<4, 4, T> rotationZ(const Angle& angle)
    {
        auto m = identity<4, T>();

        const auto _cos = static_cast<T>(cos(angle.asRadians()));
        const auto _sin = static_cast<T>(sin(angle.asRadians()));

        m.m[0][0] = _cos; m.m[1][1] = _cos;
        m.m[0][1] = static_cast<T>(-1.0 * _sin); m.m[1][0] = _sin;

        return m;
    }

    template<typename T>
    static Mat<4, 4, T> rotation(const Vec3<Angle>& angles)
    {
        return rotationZ<T>(z(angles)) * rotationY<T>(y(angles)) * rotationX<T>(x(angles));
    }

    template<typename T>
    using Mat3 = Mat<3, 3, T>;

    template<typename T>
    using Mat4 = Mat<4, 4, T>;
}