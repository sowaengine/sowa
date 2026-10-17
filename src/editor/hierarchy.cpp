#include "editor/hierarchy.hpp"
#include <utility>

#include "ecs/components/tilemap.hpp"
#include "particles/particles2d.hpp"
#include "ui/components.hpp"

#include "core/filesystem/file_system.hpp"
#include "ecs/components/animated_sprite.hpp"
#include "ecs/components/camera2d.hpp"
#include "ecs/components/nine_patch_rect.hpp"
#include "ecs/components/parallax2d.hpp"
#include "ecs/components/sprite.hpp"
#include "ecs/components/text2d.hpp"
#include "ecs/components/transform2d.hpp"
#include "ecs/world.hpp"
#include "editor/filesystem_browser.hpp"
#include "resource/resource_registry.hpp"
#include "scene/packed_scene.hpp"
#include "scene/scene.hpp"

namespace sowa::editor {

const char *ArchetypeName(EntityArchetype type) {
	switch (type) {
	case EntityArchetype::UICanvas:
		return "UICanvas";
	case EntityArchetype::UIRect:
		return "UIRect";
	case EntityArchetype::UIRow:
		return "UIRow";
	case EntityArchetype::UIColumn:
		return "UIColumn";
	case EntityArchetype::UIGrid:
		return "UIGrid";
	case EntityArchetype::UIFlow:
		return "UIFlow";
	case EntityArchetype::UIOverlay:
		return "UIOverlay";
	case EntityArchetype::UIPanel:
		return "UIPanel";
	case EntityArchetype::UINinePatchRect:
		return "UINinePatchRect";
	case EntityArchetype::UIImage:
		return "UIImage";
	case EntityArchetype::UILabel:
		return "UILabel";
	case EntityArchetype::UIButton:
		return "UIButton";
	case EntityArchetype::UIToggle:
		return "UIToggle";
	case EntityArchetype::UISlider:
		return "UISlider";
	case EntityArchetype::UIProgressBar:
		return "UIProgressBar";
	case EntityArchetype::UIScrollView:
		return "UIScrollView";
	case EntityArchetype::UIScrollbar:
		return "UIScrollbar";
	case EntityArchetype::UITextInput:
		return "UITextInput";
	case EntityArchetype::UIList:
		return "UIList";
	case EntityArchetype::UIDropdown:
		return "UIDropdown";
	case EntityArchetype::UIPopup:
		return "UIPopup";
	case EntityArchetype::UIModal:
		return "UIModal";
	case EntityArchetype::UITabs:
		return "UITabs";
	case EntityArchetype::UITooltip:
		return "UITooltip";
	case EntityArchetype::Empty:
		return "Entity";
	case EntityArchetype::Node2D:
		return "Node2D";
	case EntityArchetype::Particles2D:
		return "Particles2D";
	case EntityArchetype::Sprite:
		return "Sprite";
	case EntityArchetype::AnimatedSprite:
		return "AnimatedSprite";
	case EntityArchetype::Tilemap:
		return "Tilemap";
	case EntityArchetype::NinePatchRect:
		return "NinePatchRect";
	case EntityArchetype::Text2D:
		return "Text2D";
	case EntityArchetype::Camera2D:
		return "Camera2D";
	case EntityArchetype::Parallax2D:
		return "Parallax2D";
	}
	return "Entity";
}

Entity PlaySelection::Update(bool playing, Entity selected, const World &world) {
	if (playing == _playing) {
		return selected;
	}
	_playing = playing;
	if (playing) {
		_path = selected.IsAlive() ? selected.GetPath() : std::string();
		return {};
	}
	const std::string path = std::exchange(_path, {});
	return path.empty() ? Entity() : world.Find(path);
}

std::string DisplayName(Entity entity) {
	if (!entity.IsAlive()) {
		return {};
	}
	std::string name = entity.GetName();
	return name.empty() ? std::string("Entity") : name;
}

bool IsSceneRoot(Entity entity) {
	return entity.IsAlive() && entity.Has<SceneRoot>() && !entity.HasParent();
}

std::string InstanceSourcePath(Entity entity, const ResourceRegistry &resources) {
	if (!entity.IsAlive()) {
		return {};
	}
	const auto *instance = entity.Get<SceneInstance>();
	if (!instance) {
		return {};
	}
	const auto packed = resources.Get<PackedScene>(instance->packed);
	return packed ? packed->path : std::string();
}

bool CanDrag(Entity entity) {
	if (!entity.IsAlive()) {
		return false;
	}
	if (IsSceneRoot(entity)) {
		return false;
	}
	if (IsInstanceLocked(entity)) {
		return false;
	}
	return true;
}

bool CanReparent(Entity entity, Entity newParent) {
	if (!entity.IsAlive() || !newParent.IsAlive()) {
		return false;
	}
	if (entity == newParent) {
		return false;
	}
	if (newParent.IsDescendantOf(entity)) {
		return false;
	}
	if (entity.GetParent() == newParent) {
		return false;
	}
	if (IsSceneRoot(entity)) {
		return false;
	}
	if (IsInstanceLocked(entity)) {
		return false;
	}
	if (IsInstanceLocked(newParent) || IsInstanceRoot(newParent)) {
		return false;
	}
	return true;
}

bool TryReparent(Entity entity, Entity newParent) {
	if (!CanReparent(entity, newParent)) {
		return false;
	}
	return Reparent(entity, newParent);
}

bool CanDelete(Entity entity) {
	if (!entity.IsAlive()) {
		return false;
	}
	if (IsSceneRoot(entity)) {
		return false;
	}
	if (IsInstanceLocked(entity)) {
		return false;
	}
	return true;
}

bool TryDelete(Entity entity) {
	if (!CanDelete(entity)) {
		return false;
	}
	entity.Destroy();
	return true;
}

bool CanCreateChild(Entity parent) {
	if (!parent.IsAlive()) {
		return false;
	}
	if (IsInstanceLocked(parent) || IsInstanceRoot(parent)) {
		return false;
	}
	return true;
}

Entity SpawnChild(Entity parent, EntityArchetype type) {
	if (!CanCreateChild(parent)) {
		return {};
	}

	if (type >= EntityArchetype::UICanvas) {
		return CreateUI(parent, ArchetypeName(type));
	}
	Entity child = parent.CreateChild(ArchetypeName(type));
	switch (type) {
	case EntityArchetype::UICanvas:
	case EntityArchetype::UIRect:
	case EntityArchetype::UIRow:
	case EntityArchetype::UIColumn:
	case EntityArchetype::UIGrid:
	case EntityArchetype::UIFlow:
	case EntityArchetype::UIOverlay:
	case EntityArchetype::UIPanel:
	case EntityArchetype::UIImage:
	case EntityArchetype::UINinePatchRect:
	case EntityArchetype::UILabel:
	case EntityArchetype::UIButton:
	case EntityArchetype::UIToggle:
	case EntityArchetype::UISlider:
	case EntityArchetype::UIProgressBar:
	case EntityArchetype::UIScrollView:
	case EntityArchetype::UIScrollbar:
	case EntityArchetype::UITextInput:
	case EntityArchetype::UIList:
	case EntityArchetype::UIDropdown:
	case EntityArchetype::UIPopup:
	case EntityArchetype::UIModal:
	case EntityArchetype::UITabs:
	case EntityArchetype::UITooltip:
	case EntityArchetype::Empty:
		break;
	case EntityArchetype::Node2D:
		child.Add<Transform2D>();
		break;
	case EntityArchetype::Particles2D:
		child.Add<Transform2D>();
		child.Add<Particles2D>();
		break;
	case EntityArchetype::AnimatedSprite:
		child.Add<Transform2D>();
		child.Add<AnimatedSprite>();
		break;
	case EntityArchetype::Sprite:
		child.Add<Transform2D>();
		child.Add<Sprite>();
		break;
	case EntityArchetype::NinePatchRect:
		child.Add<Transform2D>();
		child.Add<NinePatchRect>();
		break;
	case EntityArchetype::Text2D:
		child.Add<Transform2D>();
		child.Add<Text2D>();
		break;
	case EntityArchetype::Tilemap:
		child.Add<Transform2D>();
		child.Add<Tilemap>();
		break;
	case EntityArchetype::Camera2D:
		child.Add<Transform2D>();
		child.Add<Camera2D>();
		break;
	case EntityArchetype::Parallax2D:
		child.Add<Transform2D>();
		child.Add<Parallax2D>();
		break;
	}
	return child;
}

namespace {

Scene topLevelSceneOf(Entity entity) {
	Entity topLevelRoot;
	for (Entity current = entity; current.IsAlive(); current = current.GetParent()) {
		if (current.Has<SceneRoot>() && !current.Has<SceneInstance>()) {
			topLevelRoot = current;
		}
	}
	return Scene(topLevelRoot);
}

std::string canonicalPath(FileSystem &fs, std::string_view path) {
	const FsResult<PathData> resolved = fs.Resolve(path);
	if (!resolved) {
		return {};
	}
	return resolved.value.scheme + "://" + resolved.value.path;
}

Ref<PackedScene> findPackedScene(ResourceRegistry &resources, FileSystem &fs,
	std::string_view path) {
	Ref<PackedScene> found;
	resources.Each([&](ResourceID id, Resource &resource, std::string_view typeName) {
		if (found != nullptr || typeName != "PackedScene") {
			return;
		}
		PackedScene *packed = dynamic_cast<PackedScene *>(&resource);
		if (packed != nullptr && canonicalPath(fs, packed->path) == path) {
			found = resources.Get<PackedScene>(id);
		}
	});
	return found;
}

} // namespace

SceneInstantiationResult TryInstantiateScene(Entity parent, std::string_view path, bool playing,
	FileSystem &fs, World &world, ResourceRegistry &resources) {
	SceneInstantiationResult result;
	if (playing) {
		result.error = "Scenes cannot be instantiated while playing.";
		return result;
	}
	if (!CanCreateChild(parent)) {
		result.error = "This entity cannot receive children.";
		return result;
	}

	result.path = canonicalPath(fs, path);
	if (result.path.empty()) {
		result.error = "The scene path is invalid.";
		return result;
	}
	if (KindFromName(result.path, false) != AssetKind::Scene) {
		result.error = "Choose a .scene file.";
		return result;
	}

	Scene host = topLevelSceneOf(parent);
	if (!host.IsValid()) {
		result.error = "The parent does not belong to an open scene.";
		return result;
	}
	if (!host.SourcePath().empty() && canonicalPath(fs, host.SourcePath()) == result.path) {
		result.error = "A scene cannot instantiate itself.";
		return result;
	}

	Ref<PackedScene> packed = findPackedScene(resources, fs, result.path);
	const bool created = packed == nullptr;
	if (created) {
		packed = resources.Create<PackedScene>();
		if (packed == nullptr) {
			result.error = "A PackedScene resource could not be created.";
			return result;
		}
	}

	packed->path = result.path;
	resources.Reload(packed->Id());
	result.packed = packed->Id();
	const bool alreadyOwned = host.Owns(result.packed);
	result.instance = packed->Instantiate(world, resources, parent);
	if (!result.instance.IsAlive()) {
		result.error = "The scene could not be instantiated. See Console for details.";
		if (created) {
			resources.Remove(packed->Id());
		}
		return result;
	}

	if (!alreadyOwned && !host.Own(resources, result.packed)) {
		result.instance.Destroy();
		result.instance = {};
		result.error = "The host scene could not retain the PackedScene resource.";
		if (created) {
			resources.Remove(packed->Id());
		}
		return result;
	}
	result.ownershipAdded = !alreadyOwned;
	return result;
}

} // namespace sowa::editor
