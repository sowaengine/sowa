#pragma once

#include "core/random.hpp"
#include "core/types.hpp"

namespace sowa {

// Sequential counter, starts at 1 by default (0 is reserved as "invalid"/unset).
template <typename T = u32> class LinearIdGenerator {
  public:
	using Counter = T;

	LinearIdGenerator() : _id(1) {
	}

	explicit LinearIdGenerator(Counter start) : _id(start) {
	}

	Counter Next() {
		return _id++;
	}

  private:
	Counter _id;
};

// Random opaque IDs (not a UUID, just a random integer from a private PRNG).
class RandomIdGenerator {
  public:
	RandomIdGenerator() = default;

	explicit RandomIdGenerator(u64 seed) : _random(seed) {
	}

	u32 Next() {
		return _random.U32();
	}

	u64 NextU64() {
		return _random.U64();
	}

  private:
	Random _random;
};

} // namespace sowa
