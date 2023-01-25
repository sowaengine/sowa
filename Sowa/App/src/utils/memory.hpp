#ifndef _E_MEMORY_HPP__
#define _E_MEMORY_HPP__
#pragma once

#include "stlpch.hpp"

namespace Sowa {

template <typename T>
class Allocator {
  public:
	static std::allocator<T> &Get() {
		static std::allocator<T> allocator;
		return allocator;
	}
};

} // namespace Sowa
#endif // _E_MEMORY_HPP__