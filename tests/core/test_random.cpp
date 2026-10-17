#include <doctest/doctest.h>

#include <algorithm>
#include <array>
#include <limits>
#include <set>

#include "core/random.hpp"

using namespace sowa;

TEST_CASE("Reseeding replays signed random sequences without narrowing their range") {
	Random random(42);
	std::array<i32, 64> values32;
	std::array<i64, 64> values64;
	for (size_t i = 0; i < values32.size(); ++i) {
		values32[i] = random.I32();
		values64[i] = random.I64();
	}
	CHECK(std::any_of(values32.begin(), values32.end(),
		[](i32 value) { return value < std::numeric_limits<i16>::min(); }));
	CHECK(std::any_of(values32.begin(), values32.end(),
		[](i32 value) { return value > std::numeric_limits<i16>::max(); }));
	CHECK(std::any_of(values64.begin(), values64.end(),
		[](i64 value) { return value < std::numeric_limits<i32>::min(); }));
	CHECK(std::any_of(values64.begin(), values64.end(),
		[](i64 value) { return value > std::numeric_limits<i32>::max(); }));

	random.Seed(42);
	for (size_t i = 0; i < values32.size(); ++i) {
		CHECK(random.I32() == values32[i]);
		CHECK(random.I64() == values64[i]);
	}
}

TEST_CASE("Random::Range normalizes reversed bounds without changing the seeded sequence") {
	Random forward(1);
	Random reversed(1);
	std::set<i32> samples;
	for (int i = 0; i < 64; ++i) {
		const i32 value = forward.Range(5, 10);
		CHECK(reversed.Range(10, 5) == value);
		CHECK(value >= 5);
		CHECK(value <= 10);
		samples.insert(value);
	}
	CHECK(samples.size() > 1);
	CHECK(forward.Range(5, 5) == 5);
}

TEST_CASE("Random::RangeFloat normalizes reversed bounds without changing the seeded sequence") {
	Random forward(2);
	Random reversed(2);
	std::set<f32> samples;
	for (int i = 0; i < 64; ++i) {
		const f32 value = forward.RangeFloat(-1.f, 1.f);
		CHECK(reversed.RangeFloat(1.f, -1.f) == value);
		CHECK(value >= -1.f);
		CHECK(value < 1.f);
		samples.insert(value);
	}
	CHECK(samples.size() > 1);
	CHECK(forward.RangeFloat(5.f, 5.f) == 5.f);
}
