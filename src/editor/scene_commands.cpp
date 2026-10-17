#include "editor/scene_commands.hpp"
#include "ecs/world.hpp"
#include "resource/sprite_sheet_animation.hpp"
#include <type_traits>

#include <algorithm>
#include <memory>
#include <unordered_set>
#include <utility>

#include "ecs/components/editor_entity_id.hpp"
#include "ecs/components/transform2d.hpp"
#include "editor/gizmos.hpp"
#include "editor/inspector.hpp"
#include "resource/resource_registry.hpp"
#include "scene/scene_serializer.hpp"
#include "script/script_server.hpp"

namespace sowa::editor {
namespace {

std::string relativePath(Entity root, Entity entity) {
	if (!root.IsAlive() || !entity.IsAlive()) {
		return {};
	}
	if (root == entity) {
		return {};
	}
	const std::string rootPath = root.GetPath();
	const std::string path = entity.GetPath();
	const std::string prefix = rootPath + "/";
	if (path.starts_with(prefix)) {
		return path.substr(prefix.size());
	}
	return {};
}

bool isInDocument(Entity root, Entity entity) {
	return root.IsAlive() && entity.IsAlive() && (root == entity || entity.IsDescendantOf(root));
}

Entity resolve(const std::shared_ptr<SceneDocumentContext> &context, std::string_view path) {
	if (!context || !context->root.IsAlive()) {
		return {};
	}
	return path.empty() ? context->root : context->root.Find(path);
}

struct Locator {
	std::shared_ptr<SceneDocumentContext> context;
	mutable Entity cached;
	std::string path;
	u64 id = 0;

	Entity Get() const {
		auto isInDocument = [&](Entity entity) {
			return entity.IsAlive() && context && context->root.IsAlive() &&
				   (entity == context->root || entity.IsDescendantOf(context->root));
		};
		if (isInDocument(cached)) {
			return cached;
		}
		if (context && id != 0) {
			const auto found = context->entityById.find(id);
			if (found != context->entityById.end() && isInDocument(found->second)) {
				cached = found->second;
				return cached;
			}
		}
		cached = resolve(context, path);
		return cached;
	}
};

Locator locate(const std::shared_ptr<SceneDocumentContext> &context, Entity entity) {
	u64 id = 0;
	if (const EditorEntityId *identity = entity.Get<EditorEntityId>(); identity != nullptr) {
		id = identity->Value();
	}
	return {context, entity, context ? relativePath(context->root, entity) : std::string(), id};
}

ComponentType componentNamed(Entity entity, std::string_view name) {
	for (ComponentType type : entity.GetComponents()) {
		if (type.Name() == name) {
			return type;
		}
	}
	return {};
}

template <typename T> bool setValue(const FieldRef &ref, const FieldValue &value) {
	const T *typed = std::get_if<T>(&value);
	return typed != nullptr && ref.Set(*typed);
}

bool writeField(Entity entity, std::string_view component, std::string_view field,
	const FieldValue &value) {
	if (!entity.IsAlive()) {
		return false;
	}
	ComponentType type = componentNamed(entity, component);
	FieldRef ref = entity.GetField(type, field);
	if (!ref) {
		return false;
	}
	switch (ref.Type()) {
	case FieldType::Bool:
		return setValue<bool>(ref, value);
	case FieldType::I32:
		return setValue<i32>(ref, value);
	case FieldType::U32:
		return setValue<u32>(ref, value);
	case FieldType::F32:
		return setValue<f32>(ref, value);
	case FieldType::F64:
		return setValue<f64>(ref, value);
	case FieldType::String:
		return setValue<std::string>(ref, value);
	case FieldType::EntityRef:
		return setValue<Entity>(ref, value);
	case FieldType::Vector2:
		return setValue<Vector2>(ref, value);
	case FieldType::Vector3:
		return setValue<Vector3>(ref, value);
	case FieldType::Color:
		return setValue<Color>(ref, value);
	case FieldType::Enum:
		if (ref.Data() == nullptr || !std::holds_alternative<i32>(value)) {
			return false;
		}
		*static_cast<i32 *>(ref.Data()) = std::get<i32>(value);
		return true;
	case FieldType::ResourceRef:
		return setValue<ResourceID>(ref, value);
	}
	return false;
}

class FieldCommand final : public EditCommand {
  public:
	FieldCommand(Locator entity, std::string component, std::string field, FieldValue before,
		FieldValue after)
		: _entity(std::move(entity)), _component(std::move(component)), _field(std::move(field)),
		  _before(std::move(before)), _after(std::move(after)) {
		if (const Entity *target = std::get_if<Entity>(&_before);
			target != nullptr && _entity.context && isInDocument(_entity.context->root, *target)) {
			_beforeEntity = locate(_entity.context, *target);
		}
		if (const Entity *target = std::get_if<Entity>(&_after);
			target != nullptr && _entity.context && isInDocument(_entity.context->root, *target)) {
			_afterEntity = locate(_entity.context, *target);
		}
	}

	std::string_view Name() const override {
		return "Edit Property";
	}

	bool Undo() override {
		return writeField(_entity.Get(), _component, _field, resolvedValue(_before, _beforeEntity));
	}

	bool Redo() override {
		return writeField(_entity.Get(), _component, _field, resolvedValue(_after, _afterEntity));
	}

  private:
	FieldValue resolvedValue(const FieldValue &value, const std::optional<Locator> &entity) const {
		if (!std::holds_alternative<Entity>(value)) {
			return value;
		}
		return entity ? FieldValue(entity->Get()) : value;
	}

	Locator _entity;
	std::string _component;
	std::string _field;
	FieldValue _before;
	FieldValue _after;
	std::optional<Locator> _beforeEntity;
	std::optional<Locator> _afterEntity;
};

class RenameCommand final : public EditCommand {
  public:
	RenameCommand(Locator entity, std::string before, std::string after)
		: _entity(std::move(entity)), _before(std::move(before)), _after(std::move(after)) {
	}

	std::string_view Name() const override {
		return "Rename Entity";
	}

	bool Undo() override {
		Entity entity = current(true);
		return entity.IsAlive() && entity.SetName(_before) == _before;
	}

	bool Redo() override {
		Entity entity = current(false);
		return entity.IsAlive() && entity.SetName(_after) == _after;
	}

  private:
	Entity current(bool applied) const {
		if (Entity found = _entity.Get(); found.IsAlive()) {
			return found;
		}
		if (_entity.path.empty()) {
			return resolve(_entity.context, {});
		}
		const std::size_t slash = _entity.path.rfind('/');
		const std::string parentPath =
			slash == std::string::npos ? std::string() : _entity.path.substr(0, slash);
		Entity parent = resolve(_entity.context, parentPath);
		if (!parent.IsAlive()) {
			return {};
		}
		return parent.Find(applied ? _after : _before);
	}

	Locator _entity;
	std::string _before;
	std::string _after;
};

void collectTree(Entity root, std::vector<Entity> &out) {
	if (!root.IsAlive()) {
		return;
	}
	out.push_back(root);
	for (Entity child : root.GetChildren()) {
		collectTree(child, out);
	}
}

Entity graveyard(const std::shared_ptr<SceneDocumentContext> &context) {
	if (!context || !context->root.IsAlive()) {
		return {};
	}
	if (!context->graveyard.IsAlive()) {
		context->graveyard = Entity(context->root.Raw().world().entity());
		context->graveyard.SetName("__sowa_editor_graveyard");
		context->graveyard.SetEnabled(false);
	}
	return context->graveyard;
}

class ParkedEntity {
  public:
	ParkedEntity(std::shared_ptr<SceneDocumentContext> context, Entity entity)
		: _context(std::move(context)), _locator(locate(_context, entity)), _entity(entity),
		  _livePath(relativePath(_context->root, entity)) {
	}

	bool Park() {
		Entity entity = _entity.IsAlive() ? _entity : _locator.Get();
		if (!entity.IsAlive()) {
			entity = resolve(_context, _livePath);
		}
		if (!entity.IsAlive() || !entity.HasParent()) {
			return false;
		}
		_entity = entity;
		_locator.cached = entity;
		_livePath = relativePath(_context->root, entity);
		_parent = locate(_context, entity.GetParent());
		const std::vector<Entity> siblings = entity.GetParent().GetChildren();
		const auto it = std::find(siblings.begin(), siblings.end(), entity);
		_index = it == siblings.end() ? siblings.size()
									  : static_cast<std::size_t>(it - siblings.begin());
		_nodes.clear();
		collectTree(entity, _nodes);
		_enabled.clear();
		_enabled.reserve(_nodes.size());
		for (Entity node : _nodes) {
			_enabled.push_back(node.IsEnabled());
		}
		if (!entity.SetParent(graveyard(_context))) {
			return false;
		}
		for (Entity node : _nodes) {
			node.SetEnabled(false);
		}
		_parked = true;
		return true;
	}

	bool Restore() {
		if (!_parked || !_entity.IsAlive()) {
			return false;
		}
		Entity parent = _parent.Get();
		if (!parent.IsAlive() || !_entity.SetParent(parent)) {
			return false;
		}
		std::vector<Entity> children = parent.GetChildren();
		const auto found = std::find(children.begin(), children.end(), _entity);
		if (found != children.end()) {
			children.erase(found);
			children.insert(children.begin() +
								static_cast<std::ptrdiff_t>(std::min(_index, children.size())),
				_entity);
			parent.SetChildOrder(children);
		}
		for (std::size_t i = 0; i < _nodes.size() && i < _enabled.size(); ++i) {
			_nodes[i].SetEnabled(_enabled[i]);
		}
		_parked = false;
		return true;
	}

	bool IsParked() const {
		return _parked;
	}

	Entity Get() const {
		return _entity.IsAlive() ? _entity : _locator.Get();
	}

	void DestroyIfParked() {
		if (_parked && _entity.IsAlive()) {
			_entity.Destroy();
		}
		_parked = false;
	}

  private:
	std::shared_ptr<SceneDocumentContext> _context;
	Locator _locator;
	Entity _entity;
	std::string _livePath;
	Locator _parent;
	std::size_t _index = 0;
	std::vector<Entity> _nodes;
	std::vector<bool> _enabled;
	bool _parked = false;
};

class CreateCommand final : public EditCommand {
  public:
	CreateCommand(std::shared_ptr<SceneDocumentContext> context, Entity entity,
		ResourceRegistry *resources = nullptr, ResourceID packed = {},
		bool managesOwnership = false, std::vector<ResourceID> added = {},
		std::string label = "Create Entity")
		: _context(std::move(context)), _entity(_context, entity), _resources(resources),
		  _added(std::move(added)), _label(std::move(label)) {
		if (managesOwnership) {
			_added.push_back(packed);
		}
		for (auto id : _added) {
			_resources->Retain(id);
		}
	}

	std::string_view Name() const override {
		return _label;
	}

	bool Undo() override {
		if (!_entity.Park()) {
			return false;
		}
		for (auto id : _added) {
			Scene(_context->root).Disown(*_resources, id);
		}
		return true;
	}

	bool Redo() override {
		for (auto id : _added) {
			Scene(_context->root).Own(*_resources, id);
		}
		if (_entity.Restore()) {
			return true;
		}
		for (auto id : _added) {
			Scene(_context->root).Disown(*_resources, id);
		}
		return false;
	}

	void Discard() override {
		_entity.DestroyIfParked();
		for (auto id : _added) {
			_resources->Release(id);
		}
		_added.clear();
	}

  private:
	std::shared_ptr<SceneDocumentContext> _context;
	ParkedEntity _entity;
	ResourceRegistry *_resources = nullptr;
	std::vector<ResourceID> _added;
	std::string _label;
};

struct IncomingReference {
	Locator holder;
	std::string component;
	std::string field;
	Entity target;
};

class DeleteCommand final : public EditCommand {
  public:
	DeleteCommand(std::shared_ptr<SceneDocumentContext> context, Entity entity)
		: _context(std::move(context)), _entity(_context, entity) {
	}

	std::string_view Name() const override {
		return "Delete Entity";
	}

	bool Undo() override {
		if (!_entity.Restore()) {
			return false;
		}
		for (const IncomingReference &incoming : _incoming) {
			writeField(incoming.holder.Get(), incoming.component, incoming.field,
				FieldValue(incoming.target));
		}
		return true;
	}

	bool Redo() override {
		Entity deleting = _entity.Get();
		if (!deleting.IsAlive()) {
			return false;
		}
		captureAndClearIncoming(deleting);
		return _entity.Park();
	}

	void Discard() override {
		_entity.DestroyIfParked();
	}

  private:
	void captureAndClearIncoming(Entity deleting) {
		_incoming.clear();
		std::vector<Entity> subtree;
		collectTree(deleting, subtree);
		const std::unordered_set<Entity> deleted(subtree.begin(), subtree.end());
		std::vector<Entity> sceneEntities;
		collectTree(_context->root, sceneEntities);
		for (Entity holder : sceneEntities) {
			if (deleted.contains(holder)) {
				continue;
			}
			for (ComponentType type : holder.GetComponents()) {
				for (const FieldInfo &field : type.Fields()) {
					if (field.type != FieldType::EntityRef) {
						continue;
					}
					FieldRef ref = holder.GetField(type, field.name);
					Entity *target = ref.As<Entity>();
					if (target == nullptr || !deleted.contains(*target)) {
						continue;
					}
					_incoming.push_back(
						{locate(_context, holder), type.Name(), field.name, *target});
					*target = {};
				}
			}
		}
	}

	std::shared_ptr<SceneDocumentContext> _context;
	ParkedEntity _entity;
	std::vector<IncomingReference> _incoming;
};

class ReparentCommand final : public EditCommand {
  public:
	ReparentCommand(std::shared_ptr<SceneDocumentContext> context, Entity entity, Entity newParent)
		: _context(std::move(context)), _entity(locate(_context, entity)),
		  _oldParent(locate(_context, entity.GetParent())), _newParent(locate(_context, newParent)),
		  _beforePath(relativePath(_context->root, entity)) {
		if (const Transform2D *transform = entity.Get<Transform2D>()) {
			_beforeTransform = *transform;
		}
		const std::vector<Entity> siblings = entity.GetParent().GetChildren();
		const auto found = std::find(siblings.begin(), siblings.end(), entity);
		_oldIndex = found == siblings.end() ? siblings.size()
											: static_cast<std::size_t>(found - siblings.begin());
	}

	std::string_view Name() const override {
		return "Reparent Entity";
	}

	bool Undo() override {
		Entity entity = current(true);
		Entity parent = _oldParent.Get();
		if (!entity.IsAlive() || !parent.IsAlive() || !Reparent(entity, parent)) {
			return false;
		}
		if (_beforeTransform) {
			if (Transform2D *transform = entity.Get<Transform2D>()) {
				*transform = *_beforeTransform;
			}
		}
		std::vector<Entity> children = parent.GetChildren();
		auto found = std::find(children.begin(), children.end(), entity);
		if (found != children.end()) {
			children.erase(found);
			children.insert(children.begin() +
								static_cast<std::ptrdiff_t>(std::min(_oldIndex, children.size())),
				entity);
			parent.SetChildOrder(children);
		}
		return true;
	}

	bool Redo() override {
		Entity entity = current(false);
		Entity parent = _newParent.Get();
		if (!entity.IsAlive() || !parent.IsAlive() || !Reparent(entity, parent)) {
			return false;
		}
		if (_afterTransform) {
			if (Transform2D *transform = entity.Get<Transform2D>()) {
				*transform = *_afterTransform;
			}
		} else if (const Transform2D *transform = entity.Get<Transform2D>()) {
			_afterTransform = *transform;
		}
		_afterPath = relativePath(_context->root, entity);
		return true;
	}

  private:
	Entity current(bool applied) const {
		if (Entity found = _entity.Get(); found.IsAlive()) {
			return found;
		}
		return resolve(_context, applied ? _afterPath : _beforePath);
	}

	std::shared_ptr<SceneDocumentContext> _context;
	Locator _entity;
	Locator _oldParent;
	Locator _newParent;
	std::string _beforePath;
	std::string _afterPath;
	std::size_t _oldIndex = 0;
	std::optional<Transform2D> _beforeTransform;
	std::optional<Transform2D> _afterTransform;
};

class ScriptCommand final : public EditCommand {
  public:
	ScriptCommand(Locator entity, ScriptServer &scripts, std::string path, bool attached)
		: _entity(std::move(entity)), _scripts(&scripts), _path(std::move(path)),
		  _attached(attached) {
		if (!_attached) {
			captureFields();
		}
	}

	std::string_view Name() const override {
		return _attached ? "Attach Script" : "Remove Script";
	}

	bool Undo() override {
		return apply(!_attached);
	}

	bool Redo() override {
		return apply(_attached);
	}

  private:
	bool apply(bool attach) {
		Entity entity = _entity.Get();
		if (!entity.IsAlive()) {
			return false;
		}
		if (!attach) {
			return _scripts->RemoveScript(entity, _path);
		}
		if (!_scripts->AttachScript(entity, _path)) {
			return false;
		}
		if (!_attached) {
			for (const StoredField &stored : _storedFields) {
				const FieldValue value =
					stored.entityValue ? FieldValue(stored.entityValue->Get()) : stored.value;
				writeField(entity, stored.component, stored.field, value);
			}
		}
		return true;
	}

	struct StoredField {
		std::string component;
		std::string field;
		FieldValue value;
		std::optional<Locator> entityValue;
	};

	void captureFields() {
		Entity entity = _entity.Get();
		if (!entity.IsAlive()) {
			return;
		}
		for (ComponentType type : _scripts->ScriptComponentTypes(_path)) {
			if (!entity.Has(type)) {
				continue;
			}
			for (const FieldInfo &field : type.Fields()) {
				FieldValue value;
				if (!CaptureFieldValue(entity, type, field, value)) {
					continue;
				}
				StoredField stored{type.Name(), field.name, value, std::nullopt};
				if (const Entity *target = std::get_if<Entity>(&value);
					target != nullptr && _entity.context &&
					isInDocument(_entity.context->root, *target)) {
					stored.entityValue = locate(_entity.context, *target);
				}
				_storedFields.push_back(std::move(stored));
			}
		}
	}

	Locator _entity;
	ScriptServer *_scripts = nullptr;
	std::string _path;
	bool _attached = false;
	std::vector<StoredField> _storedFields;
};

SceneDocumentSaveState *document(std::vector<SceneDocumentSaveState> &states, Scene scene) {
	return FindSceneDocumentState(states, scene);
}

} // namespace

bool CaptureFieldValue(Entity entity, ComponentType type, const FieldInfo &field, FieldValue &out) {
	FieldRef ref = entity.GetField(type, field.name);
	if (!ref) {
		return false;
	}
	switch (field.type) {
	case FieldType::Bool:
		if (const bool *value = ref.As<bool>()) {
			out = *value;
		} else {
			return false;
		}
		break;
	case FieldType::I32:
		if (const i32 *value = ref.As<i32>()) {
			out = *value;
		} else {
			return false;
		}
		break;
	case FieldType::U32:
		if (const u32 *value = ref.As<u32>()) {
			out = *value;
		} else {
			return false;
		}
		break;
	case FieldType::F32:
		if (const f32 *value = ref.As<f32>()) {
			out = *value;
		} else {
			return false;
		}
		break;
	case FieldType::F64:
		if (const f64 *value = ref.As<f64>()) {
			out = *value;
		} else {
			return false;
		}
		break;
	case FieldType::String:
		if (const std::string *value = ref.As<std::string>()) {
			out = *value;
		} else {
			return false;
		}
		break;
	case FieldType::EntityRef:
		if (const Entity *value = ref.As<Entity>()) {
			out = *value;
		} else {
			return false;
		}
		break;
	case FieldType::Vector2:
		if (const Vector2 *value = ref.As<Vector2>()) {
			out = *value;
		} else {
			return false;
		}
		break;
	case FieldType::Vector3:
		if (const Vector3 *value = ref.As<Vector3>()) {
			out = *value;
		} else {
			return false;
		}
		break;
	case FieldType::Color:
		if (const Color *value = ref.As<Color>()) {
			out = *value;
		} else {
			return false;
		}
		break;
	case FieldType::Enum:
		if (ref.Data() == nullptr) {
			return false;
		}
		out = *static_cast<i32 *>(ref.Data());
		break;
	case FieldType::ResourceRef:
		if (const ResourceID *value = ref.As<ResourceID>()) {
			out = *value;
		} else {
			return false;
		}
		break;
	}
	return true;
}

namespace {
class TilemapCommand final : public EditCommand {
	Locator _entity;
	std::optional<Tilemap> _before, _after;

	bool apply(const std::optional<Tilemap> &value) {
		Entity entity = _entity.Get();
		if (!entity.IsAlive()) {
			return false;
		}
		if (value) {
			entity.Add<Tilemap>();
			*entity.Get<Tilemap>() = *value;
		} else {
			entity.Remove<Tilemap>();
		}
		return true;
	}

  public:
	TilemapCommand(Locator entity, std::optional<Tilemap> before, std::optional<Tilemap> after)
		: _entity(std::move(entity)), _before(std::move(before)), _after(std::move(after)) {
	}

	std::string_view Name() const override {
		return "Edit tilemap";
	}

	bool Undo() override {
		return apply(_before);
	}

	bool Redo() override {
		return apply(_after);
	}
};
} // namespace

bool RecordTilemapEdit(std::vector<SceneDocumentSaveState> &states, Scene scene, Entity entity,
	std::optional<Tilemap> before, std::optional<Tilemap> after) {
	auto *state = document(states, scene);
	if (!state || !entity.IsAlive()) {
		return false;
	}
	EnsureEditorEntityId(*state, entity);
	return state->history.PushApplied(std::make_unique<TilemapCommand>(
		locate(state->context, entity), std::move(before), std::move(after)));
}

namespace {
class ParticlesCommand final : public EditCommand {
	Locator _entity;
	std::optional<Particles2D> _before, _after;

	bool apply(const std::optional<Particles2D> &value) {
		Entity entity = _entity.Get();
		if (!entity.IsAlive()) {
			return false;
		}
		if (value) {
			entity.Add<Particles2D>();
			*entity.Get<Particles2D>() = *value;
		} else {
			entity.Remove<Particles2D>();
		}
		return true;
	}

  public:
	ParticlesCommand(Locator entity, std::optional<Particles2D> before,
		std::optional<Particles2D> after)
		: _entity(std::move(entity)), _before(std::move(before)), _after(std::move(after)) {
	}

	std::string_view Name() const override {
		return "Edit particles";
	}

	bool Undo() override {
		return apply(_before);
	}

	bool Redo() override {
		return apply(_after);
	}
};
} // namespace

bool RecordParticlesEdit(std::vector<SceneDocumentSaveState> &states, Scene scene, Entity entity,
	std::optional<Particles2D> before, std::optional<Particles2D> after) {
	auto *state = document(states, scene);
	if (!state || !entity.IsAlive()) {
		return false;
	}
	EnsureEditorEntityId(*state, entity);
	return state->history.PushApplied(std::make_unique<ParticlesCommand>(
		locate(state->context, entity), std::move(before), std::move(after)));
}

namespace {
// Keep an ECS-owned value so native copy hooks preserve vectors, strings and
// opaque components as well as reflected fields. The disabled holder never runs.
class RemoveComponentCommand final : public EditCommand {
	Locator _entity;
	ComponentType _type;
	Entity _value;
	std::vector<std::pair<std::string, Locator>> _references;

	void copy(Entity from, Entity to) {
		const auto *info = ecs_get_type_info(_type.Raw().world().c_ptr(), _type.Raw().id());
		if (info && info->size) {
			ecs_set_id(to.Raw().world().c_ptr(), to.Raw().id(), _type.Raw().id(), info->size,
				from.Get(_type));
		} else {
			to.Add(_type);
		}
	}

  public:
	RemoveComponentCommand(Locator entity, ComponentType type)
		: _entity(std::move(entity)), _type(type) {
		_value = Entity(type.Raw().world().entity());
		_value.SetEnabled(false);
		copy(_entity.Get(), _value);
		for (const auto &field : type.Fields()) {
			if (field.type != FieldType::EntityRef) {
				continue;
			}
			const auto *target = _value.GetField(type, field.name).As<Entity>();
			if (target && isInDocument(_entity.context->root, *target)) {
				_references.emplace_back(field.name, locate(_entity.context, *target));
			}
		}
	}

	std::string_view Name() const override {
		return "Remove Component";
	}

	bool Redo() override {
		auto entity = _entity.Get();
		if (!entity.Has(_type)) {
			return false;
		}
		entity.Remove(_type);
		return true;
	}

	bool Undo() override {
		auto entity = _entity.Get();
		if (!entity.IsAlive() || entity.Has(_type)) {
			return false;
		}
		copy(_value, entity);
		for (const auto &[field, target] : _references) {
			entity.GetField(_type, field).Set(target.Get());
		}
		return true;
	}

	void Discard() override {
		_value.Destroy();
	}

	~RemoveComponentCommand() override {
		_value.Destroy();
	}
};

class AddComponentCommand final : public EditCommand {
	Locator _entity;
	std::string _name;
	ScriptServer &_scripts;
	bool _addTransform;

  public:
	AddComponentCommand(Locator entity, std::string name, ScriptServer &scripts, bool addTransform)
		: _entity(std::move(entity)), _name(std::move(name)), _scripts(scripts),
		  _addTransform(addTransform) {
	}

	std::string_view Name() const override {
		return "Add component";
	}

	bool Redo() override {
		Entity entity = _entity.Get();
		auto type = _scripts.GetWorld().FindComponent(_name);
		if (!entity.IsAlive() || !type.IsValid() || entity.Has(type)) {
			return false;
		}
		if (_addTransform) {
			entity.Add<Transform2D>();
		}
		entity.Add(type);
		_scripts.ApplyDefaults(entity, type);
		return true;
	}

	bool Undo() override {
		Entity entity = _entity.Get();
		if (!entity.IsAlive()) {
			return false;
		}
		entity.Remove(_scripts.GetWorld().FindComponent(_name));
		if (_addTransform) {
			entity.Remove<Transform2D>();
		}
		return true;
	}
};

template <typename Player, typename Library> class AnimationCommand final : public EditCommand {
	Locator _entity;
	ResourceRegistry &_resources;
	ResourceID _library;
	std::optional<Player> _before, _after;
	std::string _beforeData, _afterData;
	std::vector<ResourceID> _added;

	bool apply(const std::optional<Player> &player, const std::string &data) {
		Entity entity = _entity.Get();
		auto library = _resources.Get<Library>(_library);
		if (!entity.IsAlive() || !library) {
			return false;
		}
		auto archive = Archive::Load(data);
		if (!archive.IsValid()) {
			return false;
		}
		library->Serialize(archive);
		if (player) {
			entity.Add(*player);
		} else {
			entity.Remove<Player>();
		}
		return !archive.HasErrors();
	}

  public:
	AnimationCommand(Locator entity, ResourceRegistry &resources, ResourceID library,
		std::optional<Player> before, std::optional<Player> after, std::string beforeData,
		std::string afterData, bool ownershipAdded, ResourceID createdKeyResource)
		: _entity(std::move(entity)), _resources(resources), _library(library),
		  _before(std::move(before)), _after(std::move(after)), _beforeData(std::move(beforeData)),
		  _afterData(std::move(afterData)) {
		if (ownershipAdded) {
			_added.push_back(_library);
		}
		if (createdKeyResource.IsValid()) {
			_added.push_back(createdKeyResource);
		}
		for (ResourceID resource : _added) {
			_resources.Retain(resource);
		}
	}

	std::string_view Name() const override {
		return "Edit animation";
	}

	bool Undo() override {
		if (!apply(_before, _beforeData)) {
			return false;
		}
		for (ResourceID resource : _added) {
			Scene(_entity.context->root).Disown(_resources, resource);
		}
		return true;
	}

	bool Redo() override {
		for (ResourceID resource : _added) {
			Scene scene(_entity.context->root);
			if (!scene.Owns(resource) && !scene.Own(_resources, resource)) {
				return false;
			}
		}
		return apply(_after, _afterData);
	}

	void Discard() override {
		for (ResourceID resource : _added) {
			_resources.Release(resource);
		}
		_added.clear();
	}
};
} // namespace

bool ExecuteRemoveComponent(std::vector<SceneDocumentSaveState> &states, Scene scene, Entity entity,
	ComponentType type) {
	auto *state = document(states, scene);
	if (!state || !isInDocument(scene.Root(), entity) || !CanRemoveComponent(entity, type)) {
		return false;
	}
	EnsureEditorEntityId(*state, entity);
	return state->history.Execute(
		std::make_unique<RemoveComponentCommand>(locate(state->context, entity), type));
}

bool ExecuteAddComponent(std::vector<SceneDocumentSaveState> &states, Scene scene, Entity entity,
	ComponentType type, ScriptServer &scripts) {
	auto *state = document(states, scene);
	if (!state || !entity.IsAlive() || !type.IsValid() || entity.Has(type)) {
		return false;
	}
	EnsureEditorEntityId(*state, entity);
	const std::string name = type.Name();
	const bool needsTransform = name == "AnimatedSprite" || name == "Particles2D" ||
								name == "Sprite" || name == "NinePatchRect" || name == "Tilemap" ||
								name == "Camera2D" || name == "RigidBody2D" ||
								name == "Collider2D" || name == "Raycast2D";
	return state->history.Execute(
		std::make_unique<AddComponentCommand>(locate(state->context, entity), name, scripts,
			needsTransform && !entity.Has<Transform2D>()));
}

bool RecordAnimationEdit(std::vector<SceneDocumentSaveState> &states, Scene scene, Entity entity,
	ResourceRegistry &resources, ResourceID library, std::optional<AnimationPlayer> before,
	std::optional<AnimationPlayer> after, std::string beforeData, std::string afterData,
	bool ownershipAdded, ResourceID createdKeyResource) {
	auto *state = document(states, scene);
	if (!state || !entity.IsAlive()) {
		return false;
	}
	EnsureEditorEntityId(*state, entity);
	return state->history.PushApplied(
		std::make_unique<AnimationCommand<AnimationPlayer, AnimationLibrary>>(
			locate(state->context, entity), resources, library, std::move(before), std::move(after),
			std::move(beforeData), std::move(afterData), ownershipAdded, createdKeyResource));
}

bool RecordAnimatedSpriteEdit(std::vector<SceneDocumentSaveState> &states, Scene scene,
	Entity entity, ResourceRegistry &resources, ResourceID library,
	std::optional<AnimatedSprite> before, std::optional<AnimatedSprite> after,
	std::string beforeData, std::string afterData, bool ownershipAdded,
	ResourceID createdKeyResource) {
	auto *state = document(states, scene);
	if (!state || !entity.IsAlive()) {
		return false;
	}
	EnsureEditorEntityId(*state, entity);
	return state->history.PushApplied(
		std::make_unique<AnimationCommand<AnimatedSprite, SpriteSheetAnimation>>(
			locate(state->context, entity), resources, library, std::move(before), std::move(after),
			std::move(beforeData), std::move(afterData), ownershipAdded, createdKeyResource));
}

namespace {
template <typename T> class RectangleCommand final : public EditCommand {
  public:
	RectangleCommand(Locator target, T before, T after)
		: _target(std::move(target)), _before(before), _after(after) {
	}

	std::string_view Name() const override {
		if constexpr (std::is_same_v<T, UIRect>) {
			return "Edit UI rectangle";
		}
		return "Resize nine-patch rectangle";
	}

	bool Undo() override {
		return apply(_before);
	}

	bool Redo() override {
		return apply(_after);
	}

  private:
	bool apply(const T &value) {
		Entity e = _target.Get();
		if (!e.IsAlive()) {
			return false;
		}
		auto *rect = e.Get<T>();
		if (!rect) {
			return false;
		}
		*rect = value;
		return true;
	}

	Locator _target;
	T _before, _after;
};
} // namespace

bool RecordUIRectEdit(std::vector<SceneDocumentSaveState> &states, Scene scene, Entity entity,
	const UIRect &before, const UIRect &after) {
	auto *state = document(states, scene);
	if (!state || (before.position == after.position && before.size == after.size &&
					  before.anchorMin == after.anchorMin && before.anchorMax == after.anchorMax &&
					  before.pivot == after.pivot && before.fitContentX == after.fitContentX &&
					  before.fitContentY == after.fitContentY)) {
		return false;
	}
	return state->history.PushApplied(
		std::make_unique<RectangleCommand<UIRect>>(locate(state->context, entity), before, after));
}

bool RecordNinePatchResize(std::vector<SceneDocumentSaveState> &states, Scene scene, Entity entity,
	const NinePatchRect &before, const NinePatchRect &after) {
	auto *state = document(states, scene);
	if (!state || (before.size == after.size && before.offset == after.offset)) {
		return false;
	}
	return state->history.PushApplied(std::make_unique<RectangleCommand<NinePatchRect>>(
		locate(state->context, entity), before, after));
}

bool RecordFieldEdit(std::vector<SceneDocumentSaveState> &states, Scene scene, Entity entity,
	ComponentType type, const FieldInfo &field, FieldValue before, FieldValue after) {
	SceneDocumentSaveState *state = document(states, scene);
	if (!state || before == after) {
		return false;
	}
	return state->history.PushApplied(std::make_unique<FieldCommand>(locate(state->context, entity),
		type.Name(), field.name, std::move(before), std::move(after)));
}

bool RecordRename(std::vector<SceneDocumentSaveState> &states, Scene scene, Entity entity,
	std::string before, std::string after) {
	SceneDocumentSaveState *state = document(states, scene);
	if (!state || before == after) {
		return false;
	}
	return state->history.PushApplied(std::make_unique<RenameCommand>(
		locate(state->context, entity), std::move(before), std::move(after)));
}

Entity ExecuteCreateEntity(std::vector<SceneDocumentSaveState> &states, Scene scene, Entity parent,
	EntityArchetype type) {
	SceneDocumentSaveState *state = document(states, scene);
	if (!state || !isInDocument(scene.Root(), parent)) {
		return {};
	}
	Entity entity = SpawnChild(parent, type);
	if (!entity.IsAlive()) {
		return {};
	}
	EnsureEditorEntityId(*state, entity);
	state->history.PushApplied(std::make_unique<CreateCommand>(state->context, entity));
	return entity;
}

bool RecordCreatedEntity(std::vector<SceneDocumentSaveState> &states, Scene scene, Entity entity) {
	SceneDocumentSaveState *state = document(states, scene);
	if (!state || !isInDocument(scene.Root(), entity)) {
		return false;
	}
	EnsureEditorEntityId(*state, entity);
	return state->history.PushApplied(std::make_unique<CreateCommand>(state->context, entity));
}

namespace {
void freshTreeIdentity(Entity entity) {
	entity.Remove<EditorEntityId>();
	for (Entity child : entity.GetChildren()) {
		freshTreeIdentity(child);
	}
}

constexpr std::string_view clipboardPrefix = "# Sowa Entity Clipboard v1\n";
} // namespace

Entity ExecuteDuplicateEntity(std::vector<SceneDocumentSaveState> &states, Scene scene,
	Entity entity) {
	auto *state = document(states, scene);
	if (!state || !isInDocument(scene.Root(), entity) || !CanDelete(entity) ||
		!CanCreateChild(entity.GetParent())) {
		return {};
	}
	Entity copy = entity.Duplicate(entity.GetParent());
	if (!copy.IsAlive()) {
		return {};
	}
	freshTreeIdentity(copy);
	EnsureEditorEntityId(*state, copy);
	state->history.PushApplied(std::make_unique<CreateCommand>(state->context, copy, nullptr,
		ResourceID{}, false, std::vector<ResourceID>{}, "Duplicate Entity"));
	return copy;
}

bool IsEntityClipboard(std::string_view text) {
	return text.starts_with(clipboardPrefix);
}

std::string CopyEntity(Entity entity, ResourceRegistry &resources) {
	if (!CanDelete(entity)) {
		return {};
	}
	Entity host = entity;
	while (host.HasParent()) {
		host = host.GetParent();
	}
	if (!Scene(host).IsValid()) {
		return {};
	}
	Entity root(entity.Raw().world().entity());
	root.Add<SceneRoot>();
	root.Add<SceneSnapshotCopy>();
	root.SetName("Clipboard");
	Scene fragment(root);
	for (auto id : Scene(host).OwnedResources()) {
		fragment.Own(resources, id);
	}
	Entity copy = entity.Duplicate(root);
	if (!copy.IsAlive()) {
		UnloadScene(fragment, resources);
		return {};
	}
	fragment.SetEnabled(false);
	auto archive = Archive::Save();
	const bool saved = SaveScene(archive, fragment, resources) && !archive.HasErrors();
	std::string text = saved ? std::string(clipboardPrefix) + archive.ToString() : std::string();
	UnloadScene(fragment, resources);
	return text;
}

Entity ExecutePasteEntity(std::vector<SceneDocumentSaveState> &states, Scene scene, Entity parent,
	std::string_view text, World &world, ResourceRegistry &resources) {
	auto *state = document(states, scene);
	if (!state || !isInDocument(scene.Root(), parent) || !CanCreateChild(parent) ||
		!IsEntityClipboard(text)) {
		return {};
	}
	auto archive = Archive::Load(text);
	if (!archive.IsValid()) {
		return {};
	}
	const auto rootComponents = archive.Child("root").Child("components").Keys();
	if (std::find(rootComponents.begin(), rootComponents.end(), "SceneRoot") ==
			rootComponents.end() ||
		archive.Child("root").Elements("children").size() != 1) {
		return {};
	}
	Scene fragment = LoadScene(archive, world, resources);
	if (!fragment.IsValid()) {
		return {};
	}
	auto children = fragment.Root().GetChildren();
	if (archive.HasErrors() || children.size() != 1) {
		UnloadScene(fragment, resources);
		return {};
	}
	Entity copy = children.front();
	// The fragment loader resolves subtree references before moving its root.
	std::vector<ResourceID> added;
	for (auto id : fragment.OwnedResources()) {
		if (!scene.Owns(id) && scene.Own(resources, id)) {
			added.push_back(id);
		}
	}
	copy.SetParent(parent);
	freshTreeIdentity(copy);
	EnsureEditorEntityId(*state, copy);
	state->history.PushApplied(std::make_unique<CreateCommand>(state->context, copy, &resources,
		ResourceID{}, false, std::move(added), "Paste Entity"));
	UnloadScene(fragment, resources);
	return copy;
}

bool RecordInstantiatedScene(std::vector<SceneDocumentSaveState> &states, Scene scene,
	Entity entity, ResourceRegistry &resources, ResourceID packed, bool ownershipAdded) {
	SceneDocumentSaveState *state = document(states, scene);
	if (!state || !isInDocument(scene.Root(), entity) || !packed.IsValid()) {
		return false;
	}
	EnsureEditorEntityId(*state, entity);
	return state->history.PushApplied(std::make_unique<CreateCommand>(state->context, entity,
		&resources, packed, ownershipAdded));
}

bool ExecuteReparentEntity(std::vector<SceneDocumentSaveState> &states, Scene scene, Entity entity,
	Entity newParent) {
	SceneDocumentSaveState *state = document(states, scene);
	return state && isInDocument(scene.Root(), entity) && isInDocument(scene.Root(), newParent) &&
		   CanReparent(entity, newParent) &&
		   state->history.Execute(
			   std::make_unique<ReparentCommand>(state->context, entity, newParent));
}

bool ExecuteDeleteEntity(std::vector<SceneDocumentSaveState> &states, Scene scene, Entity entity) {
	SceneDocumentSaveState *state = document(states, scene);
	return state && isInDocument(scene.Root(), entity) && CanDelete(entity) &&
		   state->history.Execute(std::make_unique<DeleteCommand>(state->context, entity));
}

bool RecordScriptAttach(std::vector<SceneDocumentSaveState> &states, Scene scene, Entity entity,
	ScriptServer &scripts, std::string path) {
	SceneDocumentSaveState *state = document(states, scene);
	return state && state->history.PushApplied(std::make_unique<ScriptCommand>(
						locate(state->context, entity), scripts, std::move(path), true));
}

bool ExecuteScriptRemove(std::vector<SceneDocumentSaveState> &states, Scene scene, Entity entity,
	ScriptServer &scripts, std::string path) {
	SceneDocumentSaveState *state = document(states, scene);
	return state && state->history.Execute(std::make_unique<ScriptCommand>(
						locate(state->context, entity), scripts, std::move(path), false));
}

} // namespace sowa::editor

namespace sowa::editor {
namespace {
// A new resource is kept by history while undone, and by the scene while assigned.
class ResourceAssignmentCommand final : public EditCommand {
  public:
	ResourceAssignmentCommand(Locator entity, ComponentType type, const FieldInfo &field,
		ResourceRegistry &resources, ResourceID before, ResourceID after, bool created)
		: _field(entity, type.Name(), field.name, before, after), _context(entity.context),
		  _resources(resources), _created(created ? after : ResourceID{}) {
		if (_created.IsValid()) {
			_resources.Retain(_created);
		}
	}

	std::string_view Name() const override {
		return "Assign resource";
	}

	bool Undo() override {
		if (!_field.Undo()) {
			return false;
		}
		if (_created.IsValid()) {
			Scene(_context->root).Disown(_resources, _created);
		}
		return true;
	}

	bool Redo() override {
		if (!_field.Redo()) {
			return false;
		}
		if (_created.IsValid()) {
			Scene(_context->root).Own(_resources, _created);
		}
		return true;
	}

	void Discard() override {
		if (_created.IsValid()) {
			_resources.Release(_created);
			_created = {};
		}
	}

  private:
	FieldCommand _field;
	std::shared_ptr<SceneDocumentContext> _context;
	ResourceRegistry &_resources;
	ResourceID _created;
};

class ResourceEditCommand final : public EditCommand {
  public:
	ResourceEditCommand(std::shared_ptr<SceneDocumentContext> context, ResourceRegistry &resources,
		ResourceID resource, std::string before, std::string after, std::vector<ResourceID> added)
		: _context(std::move(context)), _resources(resources), _resource(resource),
		  _before(std::move(before)), _after(std::move(after)), _added(std::move(added)) {
		_resources.Retain(_resource);
		for (ResourceID id : _added) {
			_resources.Retain(id);
		}
	}

	std::string_view Name() const override {
		return "Edit resource";
	}

	bool Undo() override {
		if (!apply(_before)) {
			return false;
		}
		for (ResourceID id : _added) {
			Scene(_context->root).Disown(_resources, id);
		}
		return true;
	}

	bool Redo() override {
		for (ResourceID id : _added) {
			Scene(_context->root).Own(_resources, id);
		}
		return apply(_after);
	}

	void Discard() override {
		_resources.Release(_resource);
		for (ResourceID id : _added) {
			_resources.Release(id);
		}
	}

  private:
	bool apply(const std::string &data) {
		auto resource = _resources.Get(_resource);
		if (!resource) {
			return false;
		}
		auto archive = Archive::Load(data);
		resource->Serialize(archive);
		return _resources.Reload(_resource);
	}

	std::shared_ptr<SceneDocumentContext> _context;
	ResourceRegistry &_resources;
	ResourceID _resource;
	std::string _before;
	std::string _after;
	std::vector<ResourceID> _added;
};
} // namespace

bool RecordResourceAssignment(std::vector<SceneDocumentSaveState> &states, Scene scene,
	Entity entity, ComponentType type, const FieldInfo &field, ResourceRegistry &resources,
	ResourceID before, ResourceID after, bool created) {
	auto *state = document(states, scene);
	if (!state || before == after) {
		return false;
	}
	EnsureEditorEntityId(*state, entity);
	return state->history.PushApplied(std::make_unique<ResourceAssignmentCommand>(
		locate(state->context, entity), type, field, resources, before, after, created));
}

bool RecordResourceEdit(std::vector<SceneDocumentSaveState> &states, Scene scene,
	ResourceRegistry &resources, ResourceID resource, std::string before, std::string after,
	ResourceID created) {
	if (before == after || !document(states, scene)) {
		return false;
	}
	// Keep nested resources in every owning scene's saved resource list.
	std::vector<ResourceID> dependencies{resource};
	for (size_t index = 0; index < dependencies.size(); ++index) {
		auto parent = resources.Get(dependencies[index]);
		if (!parent) {
			continue;
		}
		for (const auto &field : parent->Fields()) {
			if (field.type != FieldType::ResourceRef || !field.IsSerialized()) {
				continue;
			}
			FieldRef reference(reinterpret_cast<u8 *>(parent.get()) + field.offset, &field);
			const ResourceID child = *reference.As<ResourceID>();
			if (resources.Has(child) &&
				std::find(dependencies.begin(), dependencies.end(), child) == dependencies.end()) {
				dependencies.push_back(child);
			}
		}
	}

	bool recorded = false;
	for (auto &state : states) {
		Scene owner(state.context->root);
		if (!owner.Owns(resource)) {
			continue;
		}
		std::vector<ResourceID> added;
		if (owner.Root() == scene.Root() && created.IsValid()) {
			added.push_back(created);
		}
		for (ResourceID dependency : dependencies) {
			if (!owner.Owns(dependency) && owner.Own(resources, dependency)) {
				added.push_back(dependency);
			}
		}
		state.history.PushApplied(std::make_unique<ResourceEditCommand>(state.context, resources,
			resource, before, after, std::move(added)));
		recorded = true;
	}
	return recorded;
}
} // namespace sowa::editor
