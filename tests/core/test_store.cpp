#include <doctest/doctest.h>

#include "core/store.hpp"

using namespace sowa;

TEST_CASE("Store basic set/get/has/remove") {
	StringStore store;
	CHECK_FALSE(store.Has("key"));
	CHECK(store.Get("key") == "");

	store.Set("key", "value");
	CHECK(store.Has("key"));
	CHECK(store.Get("key") == "value");

	store.Set("key", "overwritten");
	CHECK(store.Get("key") == "overwritten");

	store.Remove("key");
	CHECK_FALSE(store.Has("key"));
}

TEST_CASE("Store::Clear empties the store") {
	StringStore store;
	store.Set("a", "1");
	store.Set("b", "2");
	store.Clear();
	CHECK_FALSE(store.Has("a"));
	CHECK_FALSE(store.Has("b"));
	CHECK(store.GetStore().empty());
}

TEST_CASE("Store works with non-string value types") {
	Store<std::string, int> intStore;
	intStore.Set("count", 42);
	CHECK(intStore.Get("count") == 42);
	CHECK(intStore.Get("missing") == 0);
}
