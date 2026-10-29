#include "scene/scene_serializer.hpp"
#include "scene/tilemap_scenes.hpp"

#include <algorithm>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "core/filesystem/file_system.hpp"
#include "core/log.hpp"
#include "core/profiler.hpp"
#include "core/reflection/field_ref.hpp"
#include "core/scope_exit.hpp"
#include "ecs/component_serializer.hpp"
#include "ecs/component_type.hpp"
#include "ecs/components/scene_root.hpp"
#include "ecs/entity.hpp"
#include "ecs/world.hpp"
#include "scene/packed_scene.hpp"
#include "scene/scene.hpp"
#include "scene/scene_snapshot.hpp"

namespace sowa {

namespace {

u64 mapResourceId(u64 id, void *context) {
	const auto *remap = static_cast<const ResourceRemap *>(context);
	if (remap == nullptr) {
		return id;
	}
	const auto found = remap->find(id);
	return found != remap->end() ? found->second : id;
}

// An entity reference met while loading, held until every entity exists.
struct PendingRef {
	Entity holder;
	ComponentType type;
	std::string fieldName;
	std::string path;
};

// Records entity paths during loading for resolution after all entities exist.
// Saving uses the normal path serializer.
void collectEntityRef(void *context, Archive &archive, const FieldInfo &field, FieldRef ref) {
	auto *pending = static_cast<std::vector<PendingRef> *>(context);

	Entity *target = ref.As<Entity>();
	if (target == nullptr) {
		return;
	}

	if (archive.IsSaving()) {
		std::string path = target->IsAlive() ? target->GetPath() : std::string();
		archive.Field(field.name, path);
		return;
	}

	std::string path;
	if (!archive.Field(field.name, path) || path.empty()) {
		return;
	}

	// The holder and its component are filled in by the caller, which is the
	// only place that knows them.
	pending->push_back(PendingRef{Entity(), ComponentType(), field.name, path});
}

void saveEntity(Archive &archive, Entity entity) {
	std::string name = entity.GetName();
	archive.Field("name", name);

	std::vector<std::string> groups = entity.GetGroups();
	if (!groups.empty()) {
		archive.List("groups", groups,
			[](Archive &element, std::string &group) { element.Value(group); });
	}

	// Sort by stable component names for deterministic output across registration orders.
	std::vector<ComponentType> types = entity.GetComponents();
	std::sort(types.begin(), types.end(),
		[](const ComponentType &a, const ComponentType &b) { return a.Name() < b.Name(); });

	Archive components = archive.Child("components");
	for (const ComponentType &type : types) {
		Archive component = components.Child(type.Name());
		SerializeComponent(component, entity, type);
	}

	if (const SceneInstance *instance = entity.Get<SceneInstance>()) {
		if (instance->packed.IsValid()) {
			u64 packedId = instance->packed.value;
			archive.Field("instance", packedId);
			return;
		}
	}

	std::vector<Entity> children = entity.GetChildren();
	std::erase_if(children, [](Entity child) { return child.Has<TileSceneCell>(); });
	if (children.empty()) {
		return;
	}

	archive.List("children", children,
		[](Archive &element, Entity &child) { saveEntity(element, child); });
}

void loadEntityFields(Archive &archive, Entity entity, World &world,
	std::vector<PendingRef> &pending, const ResourceRemap *remap = nullptr) {
	std::string name;
	if (archive.Field("name", name) && !name.empty()) {
		entity.SetName(name);
	}

	std::vector<std::string> groups;
	archive.List("groups", groups,
		[](Archive &element, std::string &group) { element.Value(group); });
	for (const std::string &group : groups) {
		entity.AddGroup(group);
	}

	Archive components = archive.Child("components");
	for (const std::string &typeName : components.Keys()) {
		const ComponentType type = world.FindComponent(typeName);
		if (!type.IsValid()) {
			// Report unknown components so missing registrations do not silently lose scene data.
			components.AddError(typeName, "no component type is registered under this name");
			continue;
		}

		Archive component = components.Child(typeName);

		const std::size_t before = pending.size();
		SerializeComponent(component, entity, type, &collectEntityRef, &pending,
			remap != nullptr ? &mapResourceId : nullptr, const_cast<ResourceRemap *>(remap));
		// Anything the fallback recorded belongs to this entity and component.
		for (std::size_t i = before; i < pending.size(); ++i) {
			pending[i].holder = entity;
			pending[i].type = type;
		}
	}
}

void loadEntity(Archive &archive, Entity entity, World &world, ResourceRegistry &resources,
	std::vector<PendingRef> &pending, const ResourceRemap *remap = nullptr);

void loadChildren(Archive &archive, Entity entity, World &world, ResourceRegistry &resources,
	std::vector<PendingRef> &pending, const ResourceRemap *remap = nullptr) {
	// Create children in document order to preserve insertion order. Elements lets each
	// child exist before its data is read.
	for (Archive element : archive.Elements("children")) {
		u64 packedId = 0;
		if (element.Field("instance", packedId) && packedId != 0) {
			if (remap != nullptr) {
				packedId = mapResourceId(packedId, const_cast<ResourceRemap *>(remap));
			}
			Ref<PackedScene> packed = resources.Get<PackedScene>(ResourceID{packedId});
			if (packed == nullptr) {
				element.AddError("instance", "no PackedScene with this id is in the registry");
				continue;
			}
			Entity instance = packed->Instantiate(world, resources, entity);
			if (!instance.IsAlive()) {
				element.AddError("instance", "PackedScene failed to instantiate");
				continue;
			}
			// Host overrides (name, transform, …) on the instance root only.
			// Children come from the template.
			loadEntityFields(element, instance, world, pending, remap);
			continue;
		}

		Entity child = entity.CreateChild();
		loadEntity(element, child, world, resources, pending, remap);
	}
}

void loadEntity(Archive &archive, Entity entity, World &world, ResourceRegistry &resources,
	std::vector<PendingRef> &pending, const ResourceRemap *remap) {
	loadEntityFields(archive, entity, world, pending, remap);
	loadChildren(archive, entity, world, resources, pending, remap);
}

std::string resourceData(Resource &resource) {
	auto archive = Archive::Save();
	resource.Serialize(archive);
	return archive.ToString();
}

} // namespace

std::vector<ResourceID> LoadResourceRecords(Archive &archive, ResourceRegistry &resources,
	ResourceRemap &remap, bool retain, const std::vector<ResourceID> &owned) {
	struct Record {
		u64 original;
		ResourceID existing;
		Ref<Resource> candidate;
		Archive data;
		bool mine;
		bool shared;
	};

	std::vector<Record> records;
	const auto previous = std::exchange(remap, {});
	std::unordered_set<u64> seen;
	// Candidates need registry metadata; discard those that are not adopted.
	std::unordered_set<u64> adopted;
	auto cleanup = MakeScopeExit([&] {
		for (auto &record : records) {
			if (!adopted.contains(record.candidate->Id().value)) {
				resources.Remove(record.candidate->Id());
			}
		}
	});

	for (Archive element : archive.Elements("resources")) {
		u64 rawId = 0;
		std::string typeName;
		if (!element.Field("id", rawId) || rawId == 0 || !element.Field("type", typeName)) {
			element.AddError("id", "a resource record needs a positive id and a type");
			continue;
		}
		if (!seen.insert(rawId).second) {
			element.AddError("id", "duplicate resource id");
			continue;
		}
		ResourceID id{rawId};
		if (auto prior = previous.find(rawId); prior != previous.end() &&
			std::find(owned.begin(), owned.end(), ResourceID{prior->second}) != owned.end() &&
			resources.Has(ResourceID{prior->second})) {
			id = ResourceID{prior->second};
			remap[rawId] = id.value;
		}
		const bool existing = resources.Has(id);
		if (existing && resources.TypeNameOf(id) != typeName) {
			element.AddError("type",
				"resource id is already in use by a '" + std::string(resources.TypeNameOf(id)) +
					"'");
			continue;
		}
		auto candidate = resources.Create(typeName, existing ? ResourceID{} : id);
		if (!candidate) {
			element.AddError("type", "could not create a resource of type '" + typeName + "'");
			continue;
		}
		const bool mine = std::find(owned.begin(), owned.end(), id) != owned.end();
		records.push_back({rawId, id, candidate, element.Child("data"), mine, existing && !mine});
	}

	// Propagate remaps through dependencies until sharing decisions stop changing.
	// Each pass can only make shared records private, including cycles.
	bool changed;
	do {
		changed = false;
		for (auto &record : records) {
			auto data = record.data.WithResourceMapper(&mapResourceId, &remap);
			record.candidate->Serialize(data);
			if (record.shared &&
				resourceData(*record.candidate) != resourceData(*resources.Get(record.existing))) {
				record.shared = false;
				remap[record.original] = record.candidate->Id().value;
				changed = true;
			}
		}
	} while (changed);

	std::vector<ResourceID> ids;
	std::vector<ResourceID> reload;
	for (auto &record : records) {
		const ResourceID target =
			record.mine || record.shared ? record.existing : record.candidate->Id();
		if (!record.shared) {
			auto data = record.data.WithResourceMapper(&mapResourceId, &remap);
			resources.Get(target)->Serialize(data);
			reload.push_back(target);
		}
		if (target == record.candidate->Id()) {
			adopted.insert(target.value);
		}
		if (!retain || resources.Retain(target)) {
			ids.push_back(target);
		}
	}
	// All reference fields and IDs must exist before Reload follows a dependency.
	for (auto id : reload) {
		resources.Reload(id);
	}
	return ids;
}

namespace {

Entity resolveInstanceRef(World &world, Entity root, const std::string &rootName,
	const std::string &path) {
	if (path.empty()) {
		return Entity();
	}
	if (path == rootName) {
		return root;
	}
	if (!rootName.empty()) {
		const std::string prefix = rootName + "/";
		if (path.size() > prefix.size() && path.compare(0, prefix.size(), prefix) == 0) {
			return root.Find(path.substr(prefix.size()));
		}
	}
	return world.Find(path);
}

// Snapshot only the root's authored fields, never its children.
std::string rootFields(Entity entity) {
	Archive archive = Archive::Save();
	saveEntity(archive, entity);
	YAML::Node fields = YAML::Load(archive.ToString());
	fields.remove("children");
	fields.remove("instance");
	// Entity references within an instance use a stable local prefix in the
	// defaults snapshot, independent of its parent or uniquified root name.
	for (const auto &type : entity.GetComponents()) {
		for (const auto &field : type.Fields()) {
			if (field.type != FieldType::EntityRef) {
				continue;
			}
			auto value = fields["components"][std::string(type.Name())][field.name];
			if (!value.IsScalar()) {
				continue;
			}
			const std::string path = value.as<std::string>();
			const std::string rootPath = entity.GetPath();
			if (path == rootPath || path.starts_with(rootPath + "/")) {
				value = "$root" + path.substr(rootPath.size());
			}
		}
	}
	return YAML::Dump(fields);
}

// Apply only host edits relative to the previous inherited defaults.
// Maps merge by field, sequences are atomic; removed keys stay removed.
void mergeOverrides(const YAML::Node &before, const YAML::Node &current, YAML::Node next) {
	if (!current.IsMap()) {
		return;
	}
	for (const auto &entry : current) {
		const std::string key = entry.first.as<std::string>();
		if (key == "version") {
			continue;
		}
		const YAML::Node old = before.IsMap() ? before[key] : YAML::Node();
		const YAML::Node value = entry.second;
		// A missing key from a const YAML map is an invalid node, not a
		// null node: even IsMap() throws until its presence is checked.
		if (old && old.IsMap() && value.IsMap() && next[key].IsMap()) {
			mergeOverrides(old, value, next[key]);
		} else if (!old || YAML::Dump(old) != YAML::Dump(value)) {
			next[key] = YAML::Clone(value);
		}
	}
	if (before.IsMap()) {
		for (const auto &entry : before) {
			const std::string key = entry.first.as<std::string>();
			if (!current[key]) {
				next.remove(key);
			}
		}
	}
}

void clearRoot(Entity entity) {
	for (Entity child : entity.GetChildren()) {
		child.Destroy();
	}
	for (ComponentType type : entity.GetComponents()) {
		entity.Remove(type);
	}
	for (const auto &group : entity.GetGroups()) {
		entity.RemoveGroup(group);
	}
}

void resolvePending(World &world, Entity root, const std::string &savedName,
	std::vector<PendingRef> &pending) {
	for (auto &ref : pending) {
		const std::string prefix =
			ref.path == "$root" || ref.path.starts_with("$root/") ? "$root" : savedName;
		ref.holder.GetField(ref.type, ref.fieldName)
			.Set(resolveInstanceRef(world, root, prefix, ref.path));
	}
}

} // namespace

bool SaveScene(Archive &archive, Scene scene, ResourceRegistry &resources) {
	if (!archive.IsValid() || !scene.IsValid()) {
		return false;
	}

	std::vector<ResourceID> owned = scene.OwnedResources();
	if (!owned.empty()) {
		archive.List("resources", owned, [&](Archive &element, ResourceID &id) {
			Ref<Resource> resource = resources.Get(id);
			if (resource == nullptr) {
				// Tracked but gone: something removed it from the registry
				// without disowning it. Reported rather than written as a
				// record pointing at nothing.
				element.AddError("id", "scene owns a resource that is not in the registry");
				return;
			}

			element.Field("id", id.value);
			std::string typeName(resources.TypeNameOf(id));
			element.Field("type", typeName);

			Archive data = element.Child("data");
			resource->Serialize(data);
		});
	}

	Archive root = archive.Child("root");
	saveEntity(root, scene.Root());
	return true;
}

Scene LoadScene(Archive &archive, World &world, ResourceRegistry &resources,
	const std::vector<ResourceID> &owned) {
	if (!archive.IsValid()) {
		return Scene();
	}

	Archive root = archive.Child("root");
	if (!root.IsValid()) {
		return Scene();
	}

	// Resources first: an instance: child needs its PackedScene already
	// Reloaded before we can build the subtree. Own waits until the root
	// carries SceneRoot, which arrives with the entity walk.
	ResourceRemap remap;
	const std::vector<ResourceID> loadedIds =
		LoadResourceRecords(archive, resources, remap, false, owned);

	// Created bare and then filled in: the root's SceneRoot component comes
	// from the document like any other, so a file that omits it produces an
	// entity that isn't a scene - which Scene::IsValid then reports.
	Entity rootEntity = world.CreateEntity();

	Scene scene(rootEntity);

	std::string savedName;
	root.Field("name", savedName);

	std::vector<PendingRef> pending;
	loadEntity(root, rootEntity, world, resources, pending, &remap);

	for (const ResourceID id : loadedIds) {
		scene.Own(resources, id);
	}
	if (scene.IsValid()) {
		scene.Root().Get<SceneRoot>()->resourceRemap = remap;
	}

	// Resolve paths after all entities exist. Remap the saved root name to the live name,
	// which may have been uniquified because another copy is loaded.
	for (PendingRef &ref : pending) {
		const Entity target = resolveInstanceRef(world, rootEntity, savedName, ref.path);
		if (!target.IsAlive()) {
			archive.AddError(ref.fieldName,
				"entity reference '" + ref.path + "' matches no entity");
			continue;
		}

		FieldRef field = ref.holder.GetField(ref.type, ref.fieldName);
		if (!field.Set(target)) {
			archive.AddError(ref.fieldName, "entity reference could not be written back");
		}
	}

	return scene;
}

FsError SaveSceneToFile(FileSystem &fs, std::string_view path, Scene scene,
	ResourceRegistry &resources) {
	Archive archive = Archive::Save();
	if (!SaveScene(archive, scene, resources)) {
		return FsError::InvalidPath;
	}

	const std::string text = archive.ToString();
	FileData data;
	data.bytes.assign(reinterpret_cast<const std::byte *>(text.data()),
		reinterpret_cast<const std::byte *>(text.data() + text.size()));
	const FsError result = fs.Save(path, data);
	if (result == FsError::Ok &&
		(scene.SourcePath().empty() || fs.Normalize(scene.SourcePath()) == fs.Normalize(path))) {
		scene.Root().Get<SceneRoot>()->diskContent = text;
		scene.Root().Get<SceneRoot>()->resourceRemap.clear();
	}
	return result;
}

Scene LoadSceneFromFile(FileSystem &fs, std::string_view path, World &world,
	ResourceRegistry &resources) {
	SOWA_PROFILE_SCOPE("Scene load");
	const auto file = fs.Load(path);
	if (!file || !file.value) {
		Log::Error("LoadSceneFromFile: failed to read {}", path);
		return Scene();
	}

	const std::string text(reinterpret_cast<const char *>(file.value->Data()), file.value->Size());
	Archive archive = Archive::Load(text);
	if (!archive.IsValid()) {
		Log::Error("LoadSceneFromFile: {} is not valid YAML", path);
		return Scene();
	}

	Scene scene = LoadScene(archive, world, resources);
	if (scene.IsValid()) {
		scene.SetSourcePath(fs.Normalize(path));
		scene.Root().Get<SceneRoot>()->diskContent = text;
	}
	return scene;
}

Entity PackedScene::Instantiate(World &world, ResourceRegistry &resources, Entity parent) const {
	SOWA_PROFILE_SCOPE("Scene instantiate");
	static thread_local std::vector<ResourceID> instantiating;
	if (std::find(instantiating.begin(), instantiating.end(), Id()) != instantiating.end()) {
		Log::Error("PackedScene: cyclic instance dependency in '{}'", path);
		return Entity();
	}
	instantiating.push_back(Id());
	auto leave = MakeScopeExit([&] { instantiating.pop_back(); });
	if (_document.empty()) {
		Log::Error("PackedScene::Instantiate: '{}' has not been loaded", path);
		return Entity();
	}

	Archive archive = Archive::Load(_document);
	if (!archive.IsValid()) {
		Log::Error("PackedScene::Instantiate: '{}' is not valid YAML", path);
		return Entity();
	}

	Archive root = archive.Child("root");
	if (!root.IsValid()) {
		Log::Error("PackedScene::Instantiate: '{}' has no root", path);
		return Entity();
	}

	std::string savedName;
	root.Field("name", savedName);

	// Build entities immediately by suspending readonly and deferral; leave parenting queued.
	const bool deferred = world.IsDeferred();
	if (deferred) {
		world.SuspendDefer();
	}

	Entity rootEntity = world.CreateEntity();
	std::vector<PendingRef> pending;
	loadEntity(root, rootEntity, world, resources, pending, &_remap);
	rootEntity.Add(SceneInstance{Id()});

	for (PendingRef &ref : pending) {
		const Entity target = resolveInstanceRef(world, rootEntity, savedName, ref.path);
		if (!target.IsAlive()) {
			archive.AddError(ref.fieldName,
				"entity reference '" + ref.path + "' matches no entity");
			continue;
		}
		FieldRef field = ref.holder.GetField(ref.type, ref.fieldName);
		if (!field.Set(target)) {
			archive.AddError(ref.fieldName, "entity reference could not be written back");
		}
	}

	rootEntity.Get<SceneInstance>()->defaults = rootFields(rootEntity);
	rootEntity.Get<SceneInstance>()->revision = _revision;

	if (deferred) {
		world.ResumeDefer();
	}

	if (parent.IsAlive()) {
		rootEntity.SetParent(parent);
	}

	return rootEntity;
}

bool PackedScene::RefreshInstance(World &world, ResourceRegistry &resources,
	Entity instance) const {
	if (!instance.IsAlive() || _document.empty() || !instance.Has<SceneInstance>()) {
		return false;
	}
	const SceneInstance previous = *instance.Get<SceneInstance>();
	if (previous.packed != Id() || previous.revision == _revision) {
		return false;
	}
	const std::string current = rootFields(instance);
	const std::string name = instance.GetName();
	const bool enabled = instance.IsEnabled();
	Archive archive = Archive::Load(_document);
	Archive root = archive.Child("root");
	std::string savedName;
	root.Field("name", savedName);
	Entity staged = world.CreateEntity();
	std::vector<PendingRef> validation;
	loadEntity(root, staged, world, resources, validation);
	staged.Destroy();
	if (archive.HasErrors()) {
		Log::Warn("Cannot refresh '{}': invalid instance data", path);
		return false;
	}
	clearRoot(instance);
	std::vector<PendingRef> pending;
	loadEntity(root, instance, world, resources, pending);
	resolvePending(world, instance, savedName, pending);
	instance.SetName(name);
	const std::string defaults = rootFields(instance);
	YAML::Node merged = YAML::Load(defaults);
	if (!previous.defaults.empty()) {
		mergeOverrides(YAML::Load(previous.defaults), YAML::Load(current), merged);
	}
	// Drop components removed by the host or the new template.
	const YAML::Node components = merged["components"];
	for (ComponentType type : instance.GetComponents()) {
		if (!components[std::string(type.Name())]) {
			instance.Remove(type);
		}
	}
	for (const auto &group : instance.GetGroups()) {
		instance.RemoveGroup(group);
	}
	Archive overrides = Archive::Load(YAML::Dump(merged));
	pending.clear();
	loadEntityFields(overrides, instance, world, pending);
	resolvePending(world, instance, instance.GetPath(), pending);
	instance.SetName(name);
	instance.Add(SceneInstance{Id(), defaults, _revision});
	Scene(instance).SetEnabled(enabled);
	return true;
}

bool ReloadSceneFromFile(FileSystem &fs, Scene scene, World &world, ResourceRegistry &resources) {
	SOWA_PROFILE_SCOPE("Scene hot reload");
	if (!scene.IsValid()) {
		return false;
	}
	const std::string path = scene.SourcePath();
	const auto file = fs.Load(path);
	if (!file || !file.value) {
		return false;
	}
	const std::string text(reinterpret_cast<const char *>(file.value->Data()), file.value->Size());
	if (scene.Root().Get<SceneRoot>()->diskContent == text) {
		return false;
	}
	Archive archive = Archive::Load(text);
	if (!archive.IsValid()) {
		return false;
	}
	const YAML::Node node = YAML::Load(text)["root"];
	if (!node.IsMap() || !node["components"].IsMap() || !node["components"]["SceneRoot"]) {
		Log::Warn("Cannot reload '{}': missing scene root", path);
		return false;
	}
	// Validate entity data in a temporary tree before replacing live content.
	Scene staged = LoadScene(archive, world, resources);
	if (!staged.IsValid() || archive.HasErrors()) {
		if (staged.IsValid()) {
			UnloadScene(staged, resources);
		} else if (staged.Root().IsAlive()) {
			staged.Root().Destroy();
		}
		Log::Warn("Cannot reload '{}': invalid scene data", path);
		return false;
	}
	const auto owned = scene.OwnedResources();
	auto remap = scene.Root().Get<SceneRoot>()->resourceRemap;
	const auto incoming = LoadResourceRecords(archive, resources, remap, false, owned);
	for (ResourceID id : incoming) {
		scene.Own(resources, id);
	}
	for (ResourceID id : owned) {
		if (std::find(incoming.begin(), incoming.end(), id) == incoming.end()) {
			scene.Disown(resources, id);
		}
	}
	const bool enabled = scene.IsEnabled();
	const u64 revision = scene.Root().Get<SceneRoot>()->reloadRevision + 1;
	UnloadScene(staged, resources);
	clearRoot(scene.Root());
	Archive root = archive.Child("root");
	std::string savedName;
	root.Field("name", savedName);
	std::vector<PendingRef> pending;
	loadEntity(root, scene.Root(), world, resources, pending, &remap);
	resolvePending(world, scene.Root(), savedName, pending);
	scene.SetSourcePath(path);
	scene.Root().Get<SceneRoot>()->diskContent = text;
	scene.Root().Get<SceneRoot>()->reloadRevision = revision;
	scene.Root().Get<SceneRoot>()->resourceRemap = std::move(remap);
	scene.SetEnabled(enabled);
	return true;
}

void UnloadScene(Scene &scene, ResourceRegistry &resources) {
	if (!scene.IsValid()) {
		return;
	}

	// Discard the sibling snapshot and its resource claims before destroying the live root.
	DiscardSnapshot(scene, resources);

	// Release resources before destroying entities while their IDs are readable.
	// Registry owner counts preserve resources retained by another scene or prefab.
	for (const ResourceID id : scene.OwnedResources()) {
		resources.Release(id);
	}

	// ChildOf destruction removes the subtree while leaving other roots intact.
	scene.Root().Destroy();
	scene = Scene();
}

} // namespace sowa
