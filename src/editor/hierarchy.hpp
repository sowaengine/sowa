#pragma once

#include <string>
#include <string_view>

#include "ecs/entity.hpp"
#include "resource/resource_id.hpp"

namespace sowa {
class FileSystem;
class ResourceRegistry;
class World;
} // namespace sowa

namespace sowa::editor {

// Component presets for the hierarchy Create menu. Each preset defines the components
// attached to a new child entity.
enum class EntityArchetype {
	Empty,
	Node2D,
	Sprite,
	AnimatedSprite,
	Text2D,
	NinePatchRect,
	Particles2D,
	Tilemap,
	Camera2D,
	Parallax2D,
	UICanvas,
	UIRect,
	UIRow,
	UIColumn,
	UIGrid,
	UIFlow,
	UIOverlay,
	UIPanel,
	UIImage,
	UINinePatchRect,
	UILabel,
	UIButton,
	UIToggle,
	UISlider,
	UIProgressBar,
	UIScrollView,
	UIScrollbar,
	UITextInput,
	UIList,
	UIDropdown,
	UIPopup,
	UIModal,
	UITabs,
	UITooltip,
};

const char *ArchetypeName(EntityArchetype type);

// Entity::GetName, or "Entity" when unnamed so a tree row is never blank.
std::string DisplayName(Entity entity);

// Preserves editor selection by path across Play/Stop because snapshot restoration
// replaces entity handles. Call once per frame: Play remembers the selection and clears
// it; Stop resolves the saved path; otherwise returns selected unchanged.
class PlaySelection {
  public:
	Entity Update(bool playing, Entity selected, const World &world);

  private:
	bool _playing = false;
	std::string _path;
};

// Top-level loaded scene root: has SceneRoot and no parent. Nested SceneRoot
// (a prefab instance) is not this - GetScenes already excludes those.
bool IsSceneRoot(Entity entity);

// The source belongs to the PackedScene resource, not SceneRoot metadata
// copied into an instance. Empty for ordinary entities or missing resources.
std::string InstanceSourcePath(Entity entity, const ResourceRegistry &resources);

bool CanDrag(Entity entity);
bool CanReparent(Entity entity, Entity newParent);
bool TryReparent(Entity entity, Entity newParent);

bool CanDelete(Entity entity);
bool TryDelete(Entity entity);

bool CanCreateChild(Entity parent);
Entity SpawnChild(Entity parent, EntityArchetype type);

struct SceneInstantiationResult {
	Entity instance;
	ResourceID packed;
	bool ownershipAdded = false;
	std::string path;
	std::string error;

	explicit operator bool() const {
		return instance.IsAlive();
	}
};

// Loads/reuses a PackedScene resource for path, makes the top-level host
// scene own it so serialization can write the instance reference, then
// instantiates its root below parent.
SceneInstantiationResult TryInstantiateScene(Entity parent, std::string_view path, bool playing,
	FileSystem &fs, World &world, ResourceRegistry &resources);

} // namespace sowa::editor
