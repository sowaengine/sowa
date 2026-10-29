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

	ResourceRemap remap = _remap;
	const std::vector<ResourceID> ids =
		LoadResourceRecords(archive, resources, remap, true, _retained);
	_remap = remap;
	const auto previous = std::move(_retained);
	_retained = ids;
	_document = document;
	++_revision;
	for (ResourceID id : previous) {
		resources.Release(id);
	}
}

} // namespace sowa
