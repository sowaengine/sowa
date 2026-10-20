#pragma once

#include "animation/animation.hpp"
#include "ecs/components/animated_sprite.hpp"
#include "ecs/components/nine_patch_rect.hpp"
#include "ecs/components/tilemap.hpp"
#include "particles/particles2d.hpp"
#include "ui/components.hpp"
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "core/reflection/field_type.hpp"
#include "ecs/entity.hpp"
#include "editor/hierarchy.hpp"
#include "editor/scene_documents.hpp"
#include "math/color.hpp"
#include "math/vector2.hpp"
#include "math/vector3.hpp"
#include "resource/resource_id.hpp"

namespace sowa {

class ScriptServer;

namespace editor {

using FieldValue = std::variant<bool, i32, u32, f32, f64, std::string, Entity, Vector2, Vector3,
	Color, ResourceID>;

bool RecordNinePatchResize(std::vector<SceneDocumentSaveState> &states, Scene scene, Entity entity,
	const NinePatchRect &before, const NinePatchRect &after);

bool RecordUIRectEdit(std::vector<SceneDocumentSaveState> &states, Scene scene, Entity entity,
	const UIRect &before, const UIRect &after);

bool CaptureFieldValue(Entity entity, ComponentType type, const FieldInfo &field, FieldValue &out);

// Record a value mutation already applied by an immediate-mode widget.
bool RecordFieldEdit(std::vector<SceneDocumentSaveState> &states, Scene scene, Entity entity,
	ComponentType type, const FieldInfo &field, FieldValue before, FieldValue after);
bool RecordParticlesEdit(std::vector<SceneDocumentSaveState> &states, Scene scene, Entity entity,
	std::optional<Particles2D> before, std::optional<Particles2D> after);

bool RecordTilemapEdit(std::vector<SceneDocumentSaveState> &states, Scene scene, Entity entity,
	std::optional<Tilemap> before, std::optional<Tilemap> after);

// Clipboard text is a self-contained scene fragment, including resource records.
std::string CopyEntity(Entity entity, ResourceRegistry &resources);
bool IsEntityClipboard(std::string_view text);
Entity ExecutePasteEntity(std::vector<SceneDocumentSaveState> &states, Scene scene, Entity parent,
	std::string_view text, World &world, ResourceRegistry &resources);
Entity ExecuteDuplicateEntity(std::vector<SceneDocumentSaveState> &states, Scene scene,
	Entity entity);
bool ExecuteRemoveComponent(std::vector<SceneDocumentSaveState> &states, Scene scene, Entity entity,
	ComponentType type);

bool ExecuteAddComponent(std::vector<SceneDocumentSaveState> &states, Scene scene, Entity entity,
	ComponentType type, ScriptServer &scripts);

bool RecordAnimationEdit(std::vector<SceneDocumentSaveState> &states, Scene scene, Entity entity,
	ResourceRegistry &resources, ResourceID library, std::optional<AnimationPlayer> before,
	std::optional<AnimationPlayer> after, std::string beforeData, std::string afterData,
	bool ownershipAdded = false, ResourceID createdKeyResource = {});

bool RecordAnimatedSpriteEdit(std::vector<SceneDocumentSaveState> &states, Scene scene,
	Entity entity, ResourceRegistry &resources, ResourceID library,
	std::optional<AnimatedSprite> before, std::optional<AnimatedSprite> after,
	std::string beforeData, std::string afterData, bool ownershipAdded = false,
	ResourceID createdKeyResource = {});

bool RecordResourceAssignment(std::vector<SceneDocumentSaveState> &states, Scene scene,
	Entity entity, ComponentType type, const FieldInfo &field, ResourceRegistry &resources,
	ResourceID before, ResourceID after, bool created = false);

bool RecordResourceEdit(std::vector<SceneDocumentSaveState> &states, Scene scene,
	ResourceRegistry &resources, ResourceID resource, std::string before, std::string after,
	ResourceID created = {});

bool RecordRename(std::vector<SceneDocumentSaveState> &states, Scene scene, Entity entity,
	std::string before, std::string after);

// Structural mutations are executed by the command itself.
Entity ExecuteCreateEntity(std::vector<SceneDocumentSaveState> &states, Scene scene, Entity parent,
	EntityArchetype type);
bool RecordCreatedEntity(std::vector<SceneDocumentSaveState> &states, Scene scene, Entity entity);
bool RecordInstantiatedScene(std::vector<SceneDocumentSaveState> &states, Scene scene,
	Entity entity, ResourceRegistry &resources, ResourceID packed, bool ownershipAdded);
bool ExecuteReparentEntity(std::vector<SceneDocumentSaveState> &states, Scene scene, Entity entity,
	Entity newParent);
bool ExecuteDeleteEntity(std::vector<SceneDocumentSaveState> &states, Scene scene, Entity entity);

// ScriptServer performs these mutations before they are recorded, just like
// inspector value widgets.
bool RecordScriptAttach(std::vector<SceneDocumentSaveState> &states, Scene scene, Entity entity,
	ScriptServer &scripts, std::string path);
bool ExecuteScriptRemove(std::vector<SceneDocumentSaveState> &states, Scene scene, Entity entity,
	ScriptServer &scripts, std::string path);

struct BulkFieldEdit {
	Entity entity;
	std::string component, field;
	FieldValue before, after;
	std::optional<FieldInfo> schema;
};

bool CanEditSelection(const EntitySelection &selection, Scene scene, bool structural = false);
bool ExecuteDeleteSelection(std::vector<SceneDocumentSaveState> &states, Scene scene);
bool ExecuteDuplicateSelection(std::vector<SceneDocumentSaveState> &states, Scene scene);
bool CanReparentSelection(const EntitySelection &selection, Scene scene, Entity parent);
bool ExecuteReparentSelection(std::vector<SceneDocumentSaveState> &states, Scene scene,
	Entity parent);
bool ExecuteBulkFieldEdit(std::vector<SceneDocumentSaveState> &states, Scene scene,
	const std::vector<BulkFieldEdit> &edits, ResourceRegistry *resources = nullptr,
	ResourceID created = {});
bool RecordBulkFieldEdit(std::vector<SceneDocumentSaveState> &states, Scene scene,
	const std::vector<BulkFieldEdit> &edits);
} // namespace editor
} // namespace sowa
