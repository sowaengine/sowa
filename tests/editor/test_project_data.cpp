#include <doctest/doctest.h>

#include "editor/project_data.hpp"

using namespace sowa;

TEST_CASE("GlobalStoreEntries sorts the unordered store by key") {
	StringStore store;
	store.Set("zebra", "last");
	store.Set("alpha", "first");
	store.Set("middle", "second");

	const auto entries = editor::GlobalStoreEntries(store);
	REQUIRE(entries.size() == 3);
	CHECK(entries[0].key == "alpha");
	CHECK(entries[0].value == "first");
	CHECK(entries[1].key == "middle");
	CHECK(entries[2].key == "zebra");
}

TEST_CASE("global store editor mutations reject empty and duplicate keys") {
	StringStore store;
	store.Set("lives", "3");

	CHECK_FALSE(editor::TryAddGlobalStoreEntry(store, "", "ignored"));
	CHECK_FALSE(editor::TryAddGlobalStoreEntry(store, "lives", "99"));
	CHECK(store.Get("lives") == "3");
	CHECK(editor::TryAddGlobalStoreEntry(store, "score", "0"));
	CHECK(store.Get("score") == "0");
	CHECK(editor::TryRemoveGlobalStoreEntry(store, "lives"));
	CHECK_FALSE(store.Has("lives"));
	CHECK_FALSE(editor::TryRemoveGlobalStoreEntry(store, "missing"));
}
