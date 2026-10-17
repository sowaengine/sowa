#pragma once

#include <limits>
#include <random>
#include <utility>

#include "core/types.hpp"

namespace sowa {

// Seedable PRNG built on mt19937_64. Construct with an explicit seed for
// reproducibility, or default-construct to seed from std::random_device.
class Random {
  public:
	Random() : _engine(std::random_device{}()) {
	}

	explicit Random(u64 seed) : _engine(seed) {
	}

	void Seed(u64 seed) {
		_engine.seed(seed);
	}

	i32 I32() {
		return std::uniform_int_distribution<i32>{std::numeric_limits<i32>::min(),
			std::numeric_limits<i32>::max()}(_engine);
	}

	i64 I64() {
		return std::uniform_int_distribution<i64>{std::numeric_limits<i64>::min(),
			std::numeric_limits<i64>::max()}(_engine);
	}

	u32 U32() {
		return std::uniform_int_distribution<u32>{0u, std::numeric_limits<u32>::max()}(_engine);
	}

	u64 U64() {
		return std::uniform_int_distribution<u64>{0ull, std::numeric_limits<u64>::max()}(_engine);
	}

	// Uniform float in [0, 1).
	f32 Float() {
		return std::uniform_real_distribution<f32>{0.f, 1.f}(_engine);
	}

	// Inclusive integer range [min, max]. Arguments are swapped if min > max.
	i32 Range(i32 min, i32 max) {
		if (min > max) {
			std::swap(min, max);
		}
		return std::uniform_int_distribution<i32>{min, max}(_engine);
	}

	// Float range [min, max). Arguments are swapped if min > max.
	f32 RangeFloat(f32 min, f32 max) {
		if (min > max) {
			std::swap(min, max);
		}
		return std::uniform_real_distribution<f32>{min, max}(_engine);
	}

  private:
	std::mt19937_64 _engine;
};

} // namespace sowa
