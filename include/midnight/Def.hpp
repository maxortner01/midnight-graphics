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

namespace mn
{
    using handle_t = void*;

    template<typename T>
    using Handle = Utility::TypedHandle<T, void*>;
}