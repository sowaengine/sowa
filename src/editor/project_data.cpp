#include "editor/project_data.hpp"

#include <algorithm>

namespace sowa::editor {

std::vector<GlobalStoreEntry> GlobalStoreEntries(const StringStore &store) {
	std::vector<GlobalStoreEntry> entries;
	entries.reserve(store.GetStore().size());
	for (const auto &[key, value] : store.GetStore()) {
		entries.push_back(GlobalStoreEntry{key, value});
	}
	std::sort(entries.begin(), entries.end(),
		[](const GlobalStoreEntry &a, const GlobalStoreEntry &b) { return a.key < b.key; });
	return entries;
}

bool CanAddGlobalStoreEntry(const StringStore &store, std::string_view key) {
	return !key.empty() && !store.Has(std::string(key));
}

bool TryAddGlobalStoreEntry(StringStore &store, std::string key, std::string value) {
	if (!CanAddGlobalStoreEntry(store, key)) {
		return false;
	}
	store.Set(key, value);
	return true;
}

bool TryRemoveGlobalStoreEntry(StringStore &store, std::string_view key) {
	const std::string ownedKey(key);
	if (!store.Has(ownedKey)) {
		return false;
	}
	store.Remove(ownedKey);
	return true;
}

} // namespace sowa::editor
