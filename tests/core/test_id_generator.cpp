#include <doctest/doctest.h>

#include "core/id_generator.hpp"

using namespace sowa;

TEST_CASE("LinearIdGenerator starts at 1 and increments") {
	LinearIdGenerator<u32> gen;
	CHECK(gen.Next() == 1);
	CHECK(gen.Next() == 2);
	CHECK(gen.Next() == 3);
}

TEST_CASE("LinearIdGenerator honors a custom start value") {
	LinearIdGenerator<u32> gen(100);
	CHECK(gen.Next() == 100);
	CHECK(gen.Next() == 101);
}
