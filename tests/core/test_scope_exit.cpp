#include <doctest/doctest.h>

#include <utility>

#include "core/scope_exit.hpp"

using namespace sowa;

TEST_CASE("ScopeExit invokes its cleanup once") {
	int calls = 0;
	{
		const auto cleanup = MakeScopeExit([&calls] { ++calls; });
		CHECK(calls == 0);
	}
	CHECK(calls == 1);
}

TEST_CASE("moving ScopeExit transfers cleanup ownership") {
	int calls = 0;
	{
		auto first = MakeScopeExit([&calls] { ++calls; });
		const auto second = std::move(first);
	}
	CHECK(calls == 1);
}

TEST_CASE("released ScopeExit does not invoke its cleanup") {
	int calls = 0;
	{
		auto cleanup = MakeScopeExit([&calls] { ++calls; });
		cleanup.Release();
	}
	CHECK(calls == 0);
}
