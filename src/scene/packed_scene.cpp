#include "scene/packed_scene.hpp"

#include "core/filesystem/file_system.hpp"
#include "core/log.hpp"
#include "core/serialize/archive.hpp"
#include "resource/resource_registry.hpp"

namespace sowa {
namespace {
bool validEntity(const YAML::Node &entity) {
	if (!entity.IsMap()) {
		return false;
	}
	const auto components = entity["components"];
	if (components && !components.IsMap()) {
		return false;
	}
	if (components) {
		for (const auto &component : components) {
			if (!component.second.IsMap()) {
				return false;
			}
		}
	}
	const auto children = entity["children"];
	if (children && !children.IsSequence()) {
		return false;
	}
	if (children) {
		for (const auto &child : children) {
			if (!validEntity(child)) {
				return false;
			}
		}
	}
	return true;
}
} // namespace

void PackedScene::Unload(const ResourceContext &context) {
	if (context.registry != nullptr) {
		for (const ResourceID id : _retained) {
			if (context.registry->Has(id)) {
				context.registry->Release(id);
			}
		}
	}
	_retained.clear();
	_document.clear();
	++_revision;
}

void PackedScene::Reload(const ResourceContext &context) {
	if (path.empty()) {
		Unload(context);
		return;
	}
	if (context.fs == nullptr) {
		Log::Error("PackedScene: can't load {} without a filesystem", path);
		return;
	}
	if (context.registry == nullptr) {
		Log::Error("PackedScene: can't load {} without a resource registry", path);
		return;
	}

	const auto file = context.fs->Load(path);
	if (!file || !file.value) {
		Log::Error("PackedScene: failed to read {}", path);
		return;
	}

	const std::string document(reinterpret_cast<const char *>(file.value->Data()),
		static_cast<std::size_t>(file.value->Size()));
	if (document == _document) {
		return;
	}
	Archive archive = Archive::Load(document);
	if (!archive.IsValid()) {
		Log::Error("PackedScene: {} is not valid YAML", path);
		return;
	}

	const auto root = YAML::Load(document)["root"];
	if (!validEntity(root) || !root["components"].IsMap() || !root["components"]["SceneRoot"]) {
		Log::Error("PackedScene: {} has no scene root", path);
		return;
	}
	ResourceRegistry &resources = *context.registry;
	std::vector<ResourceID> retained;

	for (Archive element : archive.Elements("resources")) {
		u64 rawId = 0;
		std::string typeName;
		if (!element.Field("id", rawId) || !element.Field("type", typeName)) {
			element.AddError("id", "a resource record needs both an id and a type");
			continue;
		}

		const ResourceID id{rawId};
		if (resources.Has(id)) {
			const std::string_view existing = resources.TypeNameOf(id);
			if (existing != typeName) {
				element.AddError("type",
					"resource id is already in use by a '" + std::string(existing) + "'");
				continue;
			}
			if (!resources.Retain(id)) {
				continue;
			}
			retained.push_back(id);
			Archive data = element.Child("data");
			resources.Get(id)->Serialize(data);
			resources.Reload(id);
			continue;
		}

		Ref<Resource> resource = resources.Create(typeName, id);
		if (resource == nullptr) {
			element.AddError("type", "could not create a resource of type '" + typeName + "'");
			continue;
		}

		Archive data = element.Child("data");
		resource->Serialize(data);
		resources.Reload(id);
		if (!resources.Retain(id)) {
			continue;
		}
		retained.push_back(id);
	}
	const auto previous = std::move(_retained);
	_retained = std::move(retained);
	_document = document;
	++_revision;
	for (ResourceID id : previous) {
		resources.Release(id);
	}
}

} // namespace sowa
