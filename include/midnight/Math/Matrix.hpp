#pragma once

#include <Def.hpp>

/*
#ifdef __x86_64__
 #include <immintrin.h>
#else   
 #include "sse2neon.h"
#endif*/

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

            for (int i = 0; i < R; i++) {
                for (int k = 0; k < C; k++) { 
                    T temp = m[i][k]; // Temporary variable for better cache locality
                    for (int j = 0; j < C2; j++) {
                        r.m[i][j] += temp * mat.m[k][j];
                    }
                }
            }

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

    template<std::size_t R, std::size_t C, typename T>
    static Mat<C, R, T> transpose(const Mat<R, C, T>& mat)
    {
        Mat<C, R, T> m;
        for (int r = 0; r < R; r++)
            for (int c = 0; c < C; c++)
                m.m[c][r] = mat.m[r][c];
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
    static Mat<4, 4, T> inv(const Mat<4, 4, T>& in)
    {
        Mat<4, 4, T> m;

        // Precompute frequently used values (minors)
        T m00 = in.m[2][2] * in.m[3][3] - in.m[2][3] * in.m[3][2];
        T m01 = in.m[2][1] * in.m[3][3] - in.m[2][3] * in.m[3][1];
        T m02 = in.m[2][1] * in.m[3][2] - in.m[2][2] * in.m[3][1];
        T m03 = in.m[2][0] * in.m[3][3] - in.m[2][3] * in.m[3][0];
        T m04 = in.m[2][0] * in.m[3][2] - in.m[2][2] * in.m[3][0];
        T m05 = in.m[2][0] * in.m[3][1] - in.m[2][1] * in.m[3][0];

        // Calculate the determinant (reuse minors for efficiency)
        T det =
            in.m[0][0] * (in.m[1][1] * m00 - in.m[1][2] * m01 + in.m[1][3] * m02) -
            in.m[0][1] * (in.m[1][0] * m00 - in.m[1][2] * m03 + in.m[1][3] * m04) +
            in.m[0][2] * (in.m[1][0] * m01 - in.m[1][1] * m03 + in.m[1][3] * m05) -
            in.m[0][3] * (in.m[1][0] * m02 - in.m[1][1] * m04 + in.m[1][2] * m05);

        // If the determinant is zero, the matrix is singular and cannot be inverted
        MIDNIGHT_ASSERT(det != 0, "Matrix is singular and cannot be inverted");

        T invDet = 1 / det;

        // Compute the cofactor matrix and multiply by invDet to get the inverse
        m.m[0][0] =  invDet * (in.m[1][1] * m00 - in.m[1][2] * m01 + in.m[1][3] * m02);
        m.m[0][1] = -invDet * (in.m[0][1] * m00 - in.m[0][2] * m01 + in.m[0][3] * m02);
        m.m[0][2] =  invDet * (in.m[0][1] * (in.m[1][2] * in.m[3][3] - in.m[1][3] * in.m[3][2]) -
                               in.m[0][2] * (in.m[1][1] * in.m[3][3] - in.m[1][3] * in.m[3][1]) +
                               in.m[0][3] * (in.m[1][1] * in.m[3][2] - in.m[1][2] * in.m[3][1]));
        m.m[0][3] = -invDet * (in.m[0][1] * (in.m[1][2] * in.m[2][3] - in.m[1][3] * in.m[2][2]) -
                               in.m[0][2] * (in.m[1][1] * in.m[2][3] - in.m[1][3] * in.m[2][1]) +
                               in.m[0][3] * (in.m[1][1] * in.m[2][2] - in.m[1][2] * in.m[2][1]));

        m.m[1][0] = -invDet * (in.m[1][0] * m00 - in.m[1][2] * m03 + in.m[1][3] * m04);
        m.m[1][1] =  invDet * (in.m[0][0] * m00 - in.m[0][2] * m03 + in.m[0][3] * m04);
        m.m[1][2] = -invDet * (in.m[0][0] * (in.m[1][2] * in.m[3][3] - in.m[1][3] * in.m[3][2]) -
                               in.m[0][2] * (in.m[1][0] * in.m[3][3] - in.m[1][3] * in.m[3][0]) +
                               in.m[0][3] * (in.m[1][0] * in.m[3][2] - in.m[1][2] * in.m[3][0]));
        m.m[1][3] =  invDet * (in.m[0][0] * (in.m[1][2] * in.m[2][3] - in.m[1][3] * in.m[2][2]) -
                               in.m[0][2] * (in.m[1][0] * in.m[2][3] - in.m[1][3] * in.m[2][0]) +
                               in.m[0][3] * (in.m[1][0] * in.m[2][2] - in.m[1][2] * in.m[2][0]));

        m.m[2][0] =  invDet * (in.m[1][0] * m01 - in.m[1][1] * m03 + in.m[1][3] * m05);
        m.m[2][1] = -invDet * (in.m[0][0] * m01 - in.m[0][1] * m03 + in.m[0][3] * m05);
        m.m[2][2] =  invDet * (in.m[0][0] * (in.m[1][1] * in.m[3][3] - in.m[1][3] * in.m[3][1]) -
                               in.m[0][1] * (in.m[1][0] * in.m[3][3] - in.m[1][3] * in.m[3][0]) +
                               in.m[0][3] * (in.m[1][0] * in.m[3][1] - in.m[1][1] * in.m[3][0]));
        m.m[2][3] = -invDet * (in.m[0][0] * (in.m[1][1] * in.m[2][3] - in.m[1][3] * in.m[2][1]) -
                               in.m[0][1] * (in.m[1][0] * in.m[2][3] - in.m[1][3] * in.m[2][0]) +
                               in.m[0][3] * (in.m[1][0] * in.m[2][1] - in.m[1][1] * in.m[2][0]));

        m.m[3][0] = -invDet * (in.m[1][0] * m02 - in.m[1][1] * m04 + in.m[1][2] * m05);
        m.m[3][1] =  invDet * (in.m[0][0] * m02 - in.m[0][1] * m04 + in.m[0][2] * m05);
        m.m[3][2] = -invDet * (in.m[0][0] * (in.m[1][1] * in.m[3][2] - in.m[1][2] * in.m[3][1]) -
                               in.m[0][1] * (in.m[1][0] * in.m[3][2] - in.m[1][2] * in.m[3][0]) +
                               in.m[0][2] * (in.m[1][0] * in.m[3][1] - in.m[1][1] * in.m[3][0]));
        m.m[3][3] =  invDet * (in.m[0][0] * (in.m[1][1] * in.m[2][2] - in.m[1][2] * in.m[2][1]) -
                               in.m[0][1] * (in.m[1][0] * in.m[2][2] - in.m[1][2] * in.m[2][0]) +
                               in.m[0][2] * (in.m[1][0] * in.m[2][1] - in.m[1][1] * in.m[2][0]));

        return m;
    }

    static Vec3f rotateQuaternion(Vec3<Angle> angles, Vec3f vec)
    {
        // Step 1: Convert Euler angles (in radians) to a quaternion
        float halfX = y(angles).asRadians() * 0.5f;
        float halfY = x(angles).asRadians() * 0.5f;
        float halfZ = z(angles).asRadians() * 0.5f;

        float cosX = cos(halfX);
        float sinX = sin(halfX);
        float cosY = cos(halfY);
        float sinY = sin(halfY);
        float cosZ = cos(halfZ);
        float sinZ = sin(halfZ);

        // Quaternion components
        float w = cosY * cosX * cosZ + sinY * sinX * sinZ;
        float x = sinY * cosX * cosZ - cosY * sinX * sinZ;
        float y = cosY * sinX * cosZ + sinY * cosX * sinZ;
        float z = cosY * cosX * sinZ - sinY * sinX * cosZ;

        // Step 2: Rotate the vector using the quaternion
        // Quaternion * Vector * Conjugate(Quaternion)
        float qVecW = 0.0f;  // The w component of the vector quaternion (vector part)
        float qVecX = Math::x(vec);
        float qVecY = Math::y(vec);
        float qVecZ = Math::z(vec);

        // Perform quaternion multiplication: q * v
        float resultW = w * qVecW - x * qVecX - y * qVecY - z * qVecZ;
        float resultX = w * qVecX + x * qVecW + y * qVecZ - z * qVecY;
        float resultY = w * qVecY - x * qVecZ + y * qVecW + z * qVecX;
        float resultZ = w * qVecZ + x * qVecY - y * qVecX + z * qVecW;

        // Conjugate of the quaternion
        float conjW = w;
        float conjX = -x;
        float conjY = -y;
        float conjZ = -z;

        // Perform quaternion multiplication: result * conjugate(q)
        Vec3f rotatedVector;
        Math::x(rotatedVector) = resultW * conjX + resultX * conjW + resultY * conjZ - resultZ * conjY;
        Math::y(rotatedVector) = resultW * conjY - resultX * conjZ + resultY * conjW + resultZ * conjX;
        Math::z(rotatedVector) = resultW * conjZ + resultX * conjY - resultY * conjX + resultZ * conjW;

        return rotatedVector;
    }

    template<typename T>
    static Mat<4, 4, T> rotationFromQuaternion(Math::Vec4f quaternion)
    {
         // Extract quaternion components
        float w = x(quaternion);
        float x = y(quaternion);
        float y = z(quaternion);
        float z = Math::w(quaternion);
        
        Mat<4, 4, T> ret;
        ret.m[0][0] = 1 - 2 * (y * y + z * z);
        ret.m[0][1] = 2 * (x * y - z * w);
        ret.m[0][2] = 2 * (x * z + y * w);
        ret.m[0][3] = 0;
        
        ret.m[1][0] = 2 * (x * y + z * w);
        ret.m[1][1] = 1 - 2 * (x * x + z * z);
        ret.m[1][2] = 2 * (y * z - x * w);
        ret.m[1][3] = 0;
        
        ret.m[2][0] = 2 * (x * z - y * w);
        ret.m[2][1] = 2 * (y * z + x * w);
        ret.m[2][2] = 1 - 2 * (x * x + y * y);
        ret.m[2][3] = 0;
        
        ret.m[3][0] = 0;
        ret.m[3][1] = 0;
        ret.m[3][2] = 0;
        ret.m[3][3] = 1;        

        return ret;
    }

    static Vec3<Angle> eulerAxisAngle(Vec3f axis, Angle angle)
    {
        double halfAngle = angle.asRadians() / 2;
        double sinHalfAngle = sin(halfAngle);
        double w = cos(halfAngle);
        double x = Math::x(axis) * sinHalfAngle;
        double y = Math::y(axis) * sinHalfAngle;
        double z = Math::z(axis) * sinHalfAngle;

        // Step 2: Convert quaternion to Euler angles
        Vec3<Angle> angles;

        // Yaw (z-axis rotation)
        Math::z(angles) = Angle::radians( atan2(2 * (y * w + x * z), 1 - 2 * (y * y + z * z)) );

        // Pitch (y-axis rotation)
        double sinp = 2 * (x * w - y * z);
        if (fabs(sinp) >= 1)
            Math::y(angles) = Angle::radians( copysign(M_PI / 2, sinp) ); // use 90 degrees if out of range
        else
            Math::y(angles) = Angle::radians( asin(sinp) );

        // Roll (x-axis rotation)
        Math::x(angles) = Angle::radians( atan2(2 * (x * w + y * z), 1 - 2 * (x * x + y * y)) );

        return angles;
    }

    template<typename T>
    static Mat<4, 4, T> rotationAxisAngle(Vec3f axis, Angle angle)
    {
        Mat<4, 4, T> ret;
    
        // Precompute trigonometric values
        float cosTheta = cos(angle.asRadians());
        float sinTheta = sin(angle.asRadians());
        float oneMinusCos = 1 - cosTheta;

        // Axis components
        float ux = Math::x(axis);
        float uy = Math::y(axis);
        float uz = Math::z(axis);

        // Populate the rotation matrix based on the formula
        ret.m[0][0] = cosTheta + ux * ux * oneMinusCos;
        ret.m[0][1] = ux * uy * oneMinusCos - uz * sinTheta;
        ret.m[0][2] = ux * uz * oneMinusCos + uy * sinTheta;
        ret.m[0][3] = 0;

        ret.m[1][0] = uy * ux * oneMinusCos + uz * sinTheta;
        ret.m[1][1] = cosTheta + uy * uy * oneMinusCos;
        ret.m[1][2] = uy * uz * oneMinusCos - ux * sinTheta;
        ret.m[1][3] = 0;

        ret.m[2][0] = uz * ux * oneMinusCos - uy * sinTheta;
        ret.m[2][1] = uz * uy * oneMinusCos + ux * sinTheta;
        ret.m[2][2] = cosTheta + uz * uz * oneMinusCos;
        ret.m[2][3] = 0;

        ret.m[3][0] = 0;
        ret.m[3][1] = 0;
        ret.m[3][2] = 0;
        ret.m[3][3] = 1;

        return ret;
    }

    static Vec3<Angle> eulerFromQuaternion(Math::Vec4f quaternion)
    {
        const auto w = Math::x(quaternion);
        const auto x = Math::y(quaternion);
        const auto y = Math::z(quaternion);
        const auto z = Math::w(quaternion);

        float roll, pitch, yaw;

        // Yaw (z-axis rotation)
        yaw = atan2(2 * (y * w + x * z), 1 - 2 * (y * y + z * z));

        // Pitch (y-axis rotation)
        double sinp = 2 * (x * w - y * z);
        if (fabs(sinp) >= 1)
            pitch = copysign(M_PI / 2, sinp); // use 90 degrees if out of range
        else
            pitch = asin(sinp);

        // Roll (x-axis rotation)
        roll = atan2(2 * (x * w + y * z), 1 - 2 * (x * x + y * y));

        return Vec3<Angle>{
            Angle::radians(pitch),
            Angle::radians(roll),
            Angle::radians(yaw)
        };

    }

    template<typename T>
    static Mat<4, 4, T> rotationUsingQuaternion(Vec3<Angle> angles)
    {
        // Calculate half angles
        const auto pitch = x(angles).asRadians();
        const auto roll  = z(angles).asRadians();
        const auto yaw   = y(angles).asRadians();

        T cy = cos(yaw * 0.5);
        T sy = sin(yaw * 0.5);
        T cp = cos(pitch * 0.5);
        T sp = sin(pitch * 0.5);
        T cr = cos(roll * 0.5);
        T sr = sin(roll * 0.5);

        // Create the quaternion
        return rotationFromQuaternion<T>({
            cr * cp * cy + sr * sp * sy, // w component
            sr * cp * cy - cr * sp * sy, // x component
            cr * sp * cy + sr * cp * sy, // y component
            cr * cp * sy - sr * sp * cy  // z component
        });
    }

    template<typename T>
    using Mat2 = Mat<2, 2, T>;

    template<typename T>
    using Mat3 = Mat<3, 3, T>;

    template<typename T>
    using Mat4 = Mat<4, 4, T>;
}