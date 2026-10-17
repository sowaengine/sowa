#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "core/store.hpp"

namespace sowa::editor {

struct GlobalStoreEntry {
	std::string key;
	std::string value;
};

// Stable ordering for the editor table.
std::vector<GlobalStoreEntry> GlobalStoreEntries(const StringStore &store);

// Empty and duplicate keys are rejected so the Add row cannot silently
// overwrite an existing project value. Existing values are edited in place
// by their own row instead.
bool CanAddGlobalStoreEntry(const StringStore &store, std::string_view key);
bool TryAddGlobalStoreEntry(StringStore &store, std::string key, std::string value);
bool TryRemoveGlobalStoreEntry(StringStore &store, std::string_view key);

} // namespace sowa::editor
