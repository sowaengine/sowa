#ifndef _E_MEMORY_HPP__
#define _E_MEMORY_HPP__
#pragma once

#include "stlpch.hpp"

namespace sowa {

template <typename T>
class Allocator {
  public:
	static std::allocator<T> &Get() {
		static std::allocator<T> allocator;
		return allocator;
	}
};

} // namespace sowa
#endif // _E_MEMORY_HPP__