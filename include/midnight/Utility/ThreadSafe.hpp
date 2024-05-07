#pragma once

#include <Def.hpp>

#include <atomic>
#include <mutex>

namespace mn::ThreadSafe
{
	template<typename T>
	struct vector
	{
		vector() :
			vec(std::make_unique<std::vector<T>>())
		{	}

		void push_back(const T& val)
		{
			std::lock_guard<std::mutex> guard(lock);
			vec->push_back(val);
		}

        std::optional<T> pop()
        {
            std::lock_guard<std::mutex> guard(lock);
            if (!vec->size()) return { };

            auto v = vec->back();
            vec->pop_back();
            return v;
        }

	private:
		mutable std::mutex lock;
		std::unique_ptr<std::vector<T>> vec;
	};
}