#pragma once

#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <vector>

#include "core/filesystem/file_system.hpp"
#include "core/id_generator.hpp"
#include "core/reflection/field_type.hpp"
#include "core/types.hpp"
#include "resource/resource.hpp"
#include "resource/resource_context.hpp"
#include "resource/resource_id.hpp"
#include "resource/resource_type_builder.hpp"
#include <unordered_set>

namespace sowa {

// Owns resources through Ref and creates them by C++ type or registered name.
// Registration populates both lookup keys; lookups never insert entries.
class ResourceRegistry {
  public:
	ResourceRegistry() = default;
	~ResourceRegistry();

	// Copying would duplicate ownership of backend resources.
	ResourceRegistry(const ResourceRegistry &) = delete;
	ResourceRegistry &operator=(const ResourceRegistry &) = delete;

	// Services for Reload/Unload. The registry always supplies itself as context.registry.
	void SetContext(const ResourceContext &context);

	const ResourceContext &Context() const {
		return _context;
	}

	// Reloads using the stored context after field edits, deserialization, or file changes.
	// Returns false if the resource is unknown.
	bool Reload(ResourceID id);
	using ReloadEvents = eventpp::CallbackList<void(ResourceID)>;

	ReloadEvents &Reloaded() {
		return _reloaded;
	}

	// Registers T and returns a field-description builder. An empty or duplicate name,
	// or an already registered type, returns an inert builder.
	template <typename T> ResourceTypeBuilder<T> RegisterType(std::string name) {
		static_assert(std::is_base_of_v<Resource, T>, "T must derive from Resource");
		static_assert(std::is_default_constructible_v<T>,
			"a resource type must be default-constructible to be created by name");

		const std::type_index index(typeid(T));
		const bool ok =
			registerType(index, std::move(name), []() -> Ref<Resource> { return MakeRef<T>(); });
		if (!ok) {
			return ResourceTypeBuilder<T>(nullptr, false);
		}

		// The builder borrows the shared field table retained by resource instances.
		return ResourceTypeBuilder<T>(_types.at(index).fields.get(), true);
	}

	// Creates and owns a resource. An unspecified ID is generated randomly; pass an ID
	// to restore a saved record. Returns null and logs for an unregistered type or taken ID.
	template <typename T> Ref<T> Create(ResourceID id = ResourceID{}) {
		static_assert(std::is_base_of_v<Resource, T>, "T must derive from Resource");

		const auto it = _types.find(std::type_index(typeid(T)));
		if (it == _types.end()) {
			logUnregisteredType(typeid(T).name());
			return nullptr;
		}

		Ref<Resource> created = it->second.create();
		if (!insert(created, id, it->second.name)) {
			return nullptr;
		}

		// The factory above made a T, so this can't be a wrong-type cast.
		return std::static_pointer_cast<T>(created);
	}

	Ref<Resource> Create(std::string_view typeName, ResourceID id = ResourceID{});

	// Null if there's no such resource. Never inserts.
	Ref<Resource> Get(ResourceID id) const;

	// Null if there's no such resource *or* it isn't a T, so a wrong-typed
	// lookup is a null check rather than a reinterpretation.
	template <typename T> Ref<T> Get(ResourceID id) const {
		static_assert(std::is_base_of_v<Resource, T>, "T must derive from Resource");
		return std::dynamic_pointer_cast<T>(Get(id));
	}

	bool Has(ResourceID id) const;
	std::vector<std::string> TypeNames() const;

	// Owner counts are shared by scenes and dependent resources such as PackedScene.
	// A created resource with no retains stays until Remove or Clear; releasing the last
	// retained owner removes it.

	// False if there's no such resource, so a caller can't retain a hole.
	bool Retain(ResourceID id);

	// Drops one claim, removing the resource when the last one goes. False
	// (and logged) if the resource is unknown or was never retained, since
	// that means a Retain went missing rather than that there's nothing to do.
	bool Release(ResourceID id);

	u32 OwnerCount(ResourceID id) const;

	// Forcibly unloads and removes a resource regardless of owner count. Existing Refs
	// keep the unloaded object alive. Returns false for an unknown ID.
	bool Remove(ResourceID id);

	void Clear();

	std::size_t Count() const {
		return _resources.size();
	}

	// Registered type name for scene serialization. Empty for an unknown resource ID.
	std::string_view TypeNameOf(ResourceID id) const;

	// Described fields for a registered type name, empty if there's no such
	// type. Lets the inspector and the script bindings ask what a type looks
	// like without having an instance of it.
	const std::vector<FieldInfo> &FieldsOf(std::string_view typeName) const;

	// Visits resources in creation order for stable scene saves. Resources are non-const
	// because Serialize supports both saving and loading.
	void Each(const std::function<void(ResourceID, Resource &, std::string_view)> &func);

  private:
	struct TypeInfo {
		std::string name;
		std::function<Ref<Resource>()> create;
		// Shared with every instance of the type. A Ref rather than a member
		// vector so a resource that outlives the registry sees an empty table
		// instead of freed memory.
		Ref<std::vector<FieldInfo>> fields;
	};

	struct Entry {
		Ref<Resource> resource;
		std::string typeName;
		u32 owners = 0;
	};

	bool registerType(std::type_index index, std::string name,
		std::function<Ref<Resource>()> create);
	bool insert(const Ref<Resource> &resource, ResourceID id, const std::string &typeName);
	ResourceID generateId();
	static void logUnregisteredType(const char *typeName);

	std::unordered_map<std::type_index, TypeInfo> _types;
	std::unordered_map<std::string, std::type_index> _typesByName;

	std::unordered_map<ResourceID, Entry> _resources;
	// Creation order, kept alongside the map purely so Each() is deterministic.
	std::vector<ResourceID> _order;

	void disconnectFiles();
	std::shared_ptr<FileSystemEvents> _fileEvents;
	FileSystemEvents::Paths::Handle _readHandle, _changeHandle;
	std::vector<ResourceID> _loading;
	std::unordered_map<ResourceID, std::unordered_set<std::string>> _files;
	ReloadEvents _reloaded;
	ResourceContext _context;
	RandomIdGenerator _ids;
};

} // namespace sowa
