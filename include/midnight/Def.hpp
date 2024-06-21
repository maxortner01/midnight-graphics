#pragma once

#include <iostream>
#include <memory>
#include <exception>
#include <cassert>
#include <vector>
#include <algorithm>
#include <span>
#include <sstream>
#include <filesystem>

#include "Utility/TypedHandle.hpp"

#define MIDNIGHT_ASSERT(expr, msg) if (!(expr)) { std::cout << "\nCritical error!\n - " << std::filesystem::path(__FILE__).filename().string() << ":" << __LINE__ << "\n - Assertion failed (" << #expr << ")\n\n" << msg << std::endl; std::terminate(); }

#ifdef _MSC_VER
#   ifdef MN_BUILD
#       define MN_SYMBOL __declspec(dllexport)
#   else
#       define MN_SYMBOL __declspec(dllimport)
#   endif
#else
    #define MN_SYMBOL
#endif

namespace mn
{
    using handle_t = void*;

    template<typename T>
    using Handle = Utility::TypedHandle<T, void*>;

    using u8 = uint8_t;
    using i8 = int8_t;

    using u16 = uint16_t;
    using i16 = int16_t;

    using u32 = uint32_t;
    using i32 = int32_t;

    using r32 = float;
    using r64 = double;
}