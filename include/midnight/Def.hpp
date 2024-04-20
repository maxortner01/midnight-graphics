#pragma once

#include <iostream>
#include <memory>
#include <exception>
#include <cassert>
#include <vector>
#include <algorithm>
#include <span>

#include "Utility/TypedHandle.hpp"

namespace mn
{
    using handle_t = void*;

    template<typename T>
    using Handle = Utility::TypedHandle<T, void*>;
}