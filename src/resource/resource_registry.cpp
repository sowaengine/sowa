#include "resource/resource_registry.hpp"

#include <algorithm>
#include <utility>

#include "core/log.hpp"
#include "core/profiler.hpp"

namespace sowa {

namespace {

ResourceContext boundContext(ResourceRegistry *registry, const ResourceContext &stored) {
	ResourceContext context = stored;
	context.registry = registry;
	return context;
}

} // namespace

ResourceRegistry::~ResourceRegistry() {
	disconnectFiles();
	Clear();
}

void ResourceRegistry::disconnectFiles() {
	if (!_fileEvents) {
		return;
	}
	_fileEvents->read.remove(_readHandle);
	_fileEvents->changed.remove(_changeHandle);
	_fileEvents.reset();
}

void ResourceRegistry::SetContext(const ResourceContext &context) {
	disconnectFiles();
	_context = context;
	if (!context.fs) {
		return;
	}
	_fileEvents = context.fs->Events();
	_readHandle = _fileEvents->read.append([this](const std::string &path) {
		if (!_loading.empty()) {
			_files[_loading.back()].insert(path);
		}
	});
	_changeHandle = _fileEvents->changed.append([this](const std::string &path) {
		std::vector<ResourceID> affected;
		for (const auto &[id, files] : _files) {
			if (files.contains(path)) {
				affected.push_back(id);
			}
		}
		for (ResourceID id : affected) {
			Reload(id);
		}
	});
}

bool ResourceRegistry::Reload(ResourceID id) {
	SOWA_PROFILE_SCOPE("Resource load");
	const auto it = _resources.find(id);
	if (it == _resources.end()) {
		return false;
	}

	// Recursive PackedScene dependencies may form a cycle. Never re-enter
	// a resource already being loaded, and retain the object across callbacks.
	if (std::find(_loading.begin(), _loading.end(), id) != _loading.end()) {
		return false;
	}
	const Ref<Resource> resource = it->second.resource;
	_files[id].clear();
	_loading.push_back(id);
	try {
		resource->Reload(boundContext(this, _context));
	} catch (...) {
		_loading.pop_back();
		throw;
	}
	_loading.pop_back();
	_reloaded(id);
	return true;
}

bool ResourceRegistry::registerType(std::type_index index, std::string name,
	std::function<Ref<Resource>()> create) {
	if (name.empty()) {
		Log::Error("ResourceRegistry: a resource type needs a name");
		return false;
	}
	if (_typesByName.find(name) != _typesByName.end()) {
		Log::Error("ResourceRegistry: resource type '{}' is already registered", name);
		return false;
	}
	if (_types.find(index) != _types.end()) {
		Log::Error("ResourceRegistry: this type is already registered as '{}'",
			_types.at(index).name);
		return false;
	}

	_typesByName.emplace(name, index);
	_types.emplace(index,
		TypeInfo{std::move(name), std::move(create), MakeRef<std::vector<FieldInfo>>()});
	return true;
}

Ref<Resource> ResourceRegistry::Create(std::string_view typeName, ResourceID id) {
	const auto named = _typesByName.find(std::string(typeName));
	if (named == _typesByName.end()) {
		Log::Error("ResourceRegistry: no resource type named '{}'", typeName);
		return nullptr;
	}

	const TypeInfo &type = _types.at(named->second);
	Ref<Resource> created = type.create();
	if (!insert(created, id, type.name)) {
		return nullptr;
	}

	return created;
}

bool ResourceRegistry::insert(const Ref<Resource> &resource, ResourceID id,
	const std::string &typeName) {
	if (!resource) {
		return false;
	}

	if (id.IsValid()) {
		if (_resources.find(id) != _resources.end()) {
			Log::Error("ResourceRegistry: resource id {} is already in use", id.value);
			return false;
		}
	} else {
		id = generateId();
	}

	resource->_id = id;
	// Attach the field table for the registered resource type.
	const auto type = _typesByName.find(typeName);
	if (type != _typesByName.end()) {
		resource->_fields = _types.at(type->second).fields;
	}

	_resources.emplace(id, Entry{resource, typeName});
	_order.push_back(id);
	return true;
}

ResourceID ResourceRegistry::generateId() {
	// Rejects 0 (reserved for "no resource") and anything already taken, so a
	// generated id can never alias one restored from a scene file.
	for (;;) {
		const u64 value = _ids.NextU64();
		if (value == 0) {
			continue;
		}

		const ResourceID id{value};
		if (_resources.find(id) == _resources.end()) {
			return id;
		}
	}
}

void ResourceRegistry::logUnregisteredType(const char *typeName) {
	Log::Error("ResourceRegistry: resource type '{}' was never registered", typeName);
}

Ref<Resource> ResourceRegistry::Get(ResourceID id) const {
	const auto it = _resources.find(id);
	if (it == _resources.end()) {
		return nullptr;
	}
	return it->second.resource;
}

bool ResourceRegistry::Has(ResourceID id) const {
	return _resources.find(id) != _resources.end();
}

bool ResourceRegistry::Retain(ResourceID id) {
	const auto it = _resources.find(id);
	if (it == _resources.end()) {
		return false;
	}

	++it->second.owners;
	return true;
}

bool ResourceRegistry::Release(ResourceID id) {
	const auto it = _resources.find(id);
	if (it == _resources.end()) {
		Log::Error("ResourceRegistry: released resource id {}, which isn't here", id.value);
		return false;
	}
	if (it->second.owners == 0) {
		// A missing Retain, not a harmless no-op: silently tolerating it would
		// hide exactly the mismatch this counter exists to catch.
		Log::Error("ResourceRegistry: released resource id {}, which nobody had retained",
			id.value);
		return false;
	}

	if (--it->second.owners > 0) {
		return true;
	}

	return Remove(id);
}

u32 ResourceRegistry::OwnerCount(ResourceID id) const {
	const auto it = _resources.find(id);
	return it != _resources.end() ? it->second.owners : 0;
}

bool ResourceRegistry::Remove(ResourceID id) {
	const auto it = _resources.find(id);
	if (it == _resources.end()) {
		return false;
	}

	// Keep the object alive while removing its entry before Unload: packed
	// resources may release children and rehash or erase registry entries.
	const Ref<Resource> resource = it->second.resource;
	_resources.erase(it);
	_files.erase(id);
	resource->Unload(boundContext(this, _context));
	// Remove stale IDs from iteration order.
	_order.erase(std::remove(_order.begin(), _order.end(), id), _order.end());
	return true;
}

void ResourceRegistry::Clear() {
	// Unload may release other entries. Iterate a copy through Remove.
	const auto order = _order;
	for (ResourceID id : order) {
		Remove(id);
	}
	_files.clear();
	_order.clear();
}

const std::vector<FieldInfo> &ResourceRegistry::FieldsOf(std::string_view typeName) const {
	static const std::vector<FieldInfo> empty;

	const auto named = _typesByName.find(std::string(typeName));
	if (named == _typesByName.end()) {
		return empty;
	}
	return *_types.at(named->second).fields;
}

std::string_view ResourceRegistry::TypeNameOf(ResourceID id) const {
	const auto it = _resources.find(id);
	if (it == _resources.end()) {
		return {};
	}
	return it->second.typeName;
}

void ResourceRegistry::Each(
	const std::function<void(ResourceID, Resource &, std::string_view)> &func) {
	if (!func) {
		return;
	}

	// Iterate over a copy so callbacks may create or remove resources without invalidating
	// the walk. Skip IDs removed by earlier callbacks.
	const std::vector<ResourceID> order = _order;
	for (const ResourceID id : order) {
		const auto it = _resources.find(id);
		if (it == _resources.end()) {
			continue;
		}
		func(id, *it->second.resource, it->second.typeName);
	}
}

} // namespace sowa

namespace sowa {
std::vector<std::string> ResourceRegistry::TypeNames() const {
	std::vector<std::string> names;
	for (const auto &[name, type] : _typesByName) {
		names.push_back(name);
	}
	std::sort(names.begin(), names.end());
	return names;
}
} // namespace sowa
