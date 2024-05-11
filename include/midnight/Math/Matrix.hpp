#pragma once

#include <Def.hpp>

#include "Vector.hpp"
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

        template<typename A>
        auto operator*(const Vec<C, A>& v) const
        {
            static_assert(R == C);
            Vec<C, A> _r;

            for (uint32_t c = 0; c < C; c++)
                for (uint32_t r = 0; r < R; r++)
                    _r.c[c] += m[r][c] * v.c[r];
            
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

    // z_clip is { near, far }
    template<typename T>
    static Mat<4, 4, T> perspective(const T& aspect, const Angle& FOV, const Vec2<T>& z_clip)
    {
        const auto& [ near, far ] = std::tie(x(z_clip), y(z_clip));
        const auto _itan = 1.0 / tan( FOV.asRadians() / 2.0 );

        Mat<4, 4, T> m;

        m.m[0][0] = _itan / aspect;
        m.m[1][1] = _itan;
        m.m[2][2] = -1.0 * (far + near) / (far - near);
        m.m[2][3] = -1;
        m.m[3][2] = -2.0 * (far * near) / (far - near);

        return m;
    }

    template<typename T>
    static Mat<4, 4, T> translation(const Vec3<T>& pos)
    {
        auto m = identity<4, T>();

        m.m[3][0] = x(pos);
        m.m[3][1] = y(pos);
        m.m[3][2] = z(pos);

        return m;
    }

    template<typename T>
    static Mat<4, 4, T> scale(const Vec3<T>& sc)
    {
        auto m = identity<4, T>();
        m.m[0][0] = sc.c[0];
        m.m[1][1] = sc.c[1];
        m.m[2][2] = sc.c[2];
        return m;
    }

    template<typename T>
    using Mat3 = Mat<3, 3, T>;

    template<typename T>
    using Mat4 = Mat<4, 4, T>;
}