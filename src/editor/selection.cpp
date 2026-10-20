#include "editor/selection.hpp"

#include <algorithm>

#include "ecs/components/editor_entity_id.hpp"

namespace sowa::editor {
namespace {

bool belongs(Entity entity, Entity root) {
	return entity.IsAlive() && (!root.IsAlive() || entity == root || entity.IsDescendantOf(root));
}

std::string relative(Entity root, Entity entity) {
	if (root == entity) {
		return {};
	}
	return entity.GetPath().substr(root.GetPath().size() + 1);
}
} // namespace

bool EntitySelection::Contains(Entity entity) const {
	return std::find(_members.begin(), _members.end(), entity) != _members.end();
}

Entity EntitySelection::ActiveRoot() const {
	Entity root = _active;
	for (Entity parent = root.GetParent(); parent.IsAlive(); parent = parent.GetParent()) {
		if (Contains(parent)) {
			root = parent;
		}
	}
	return root;
}

void EntitySelection::Clear() {
	_members.clear();
	_active = {};
	_anchor = {};
}

void EntitySelection::Set(Entity entity) {
	Click(entity);
}

void EntitySelection::Assign(std::vector<Entity> entities, Entity active, Entity anchor) {
	Clear();
	for (Entity entity : entities) {
		if (entity.IsAlive() && !Contains(entity)) {
			_members.push_back(entity);
		}
	}
	_active = Contains(active) ? active : (_members.empty() ? Entity{} : _members.back());
	_anchor = anchor.IsAlive() ? anchor : _active;
}

void EntitySelection::Activate(Entity entity) {
	if (Contains(entity)) {
		_active = entity;
	}
}

void EntitySelection::Click(Entity entity, SelectionMode mode) {
	if (mode == SelectionMode::Replace) {
		Clear();
	}
	if (!entity.IsAlive()) {
		return;
	}
	if (mode == SelectionMode::Toggle && Contains(entity)) {
		std::erase(_members, entity);
		if (_active == entity) {
			_active = _members.empty() ? Entity{} : _members.back();
		}
		_anchor = _active;
	} else {
		if (!Contains(entity)) {
			_members.push_back(entity);
		}
		_active = _anchor = entity;
	}
}

void EntitySelection::Range(Entity entity, const std::vector<Entity> &visible, bool additive) {
	auto first = std::find(visible.begin(), visible.end(), _anchor);
	auto last = std::find(visible.begin(), visible.end(), entity);
	if (first == visible.end() || last == visible.end()) {
		Click(entity, additive ? SelectionMode::Add : SelectionMode::Replace);
		return;
	}
	Entity anchor = _anchor;
	if (first > last) {
		std::swap(first, last);
	}
	if (!additive) {
		Clear();
	}
	for (auto it = first; it != last + 1; ++it) {
		if (it->IsAlive() && !Contains(*it)) {
			_members.push_back(*it);
		}
	}
	_active = entity;
	_anchor = anchor;
}

void EntitySelection::Prune(Entity root) {
	std::erase_if(_members, [&](Entity e) { return !belongs(e, root); });
	if (!Contains(_active)) {
		_active = _members.empty() ? Entity{} : _members.back();
	}
	if (!belongs(_anchor, root) || _members.empty()) {
		_anchor = _active;
	}
}

std::vector<Entity> SelectionRoots(const std::vector<Entity> &members) {
	std::vector<Entity> roots;
	for (Entity e : members) {
		if (!e.IsAlive() || std::find(roots.begin(), roots.end(), e) != roots.end()) {
			continue;
		}
		if (std::none_of(members.begin(), members.end(), [&](Entity parent) {
				return parent != e && parent.IsAlive() && e.IsDescendantOf(parent);
			})) {
			roots.push_back(e);
		}
	}
	return roots;
}

SelectionBookmark SelectionBookmark::Capture(Entity root, const EntitySelection &selection) {
	SelectionBookmark saved;
	for (Entity entity : selection.Members()) {
		if (!belongs(entity, root) || !root.IsAlive()) {
			continue;
		}
		Entry entry;
		Entity owner = entity;
		while (owner.IsAlive() && owner != root && !owner.Has<EditorEntityId>()) {
			owner = owner.GetParent();
		}
		if (const auto *id = owner.Get<EditorEntityId>()) {
			entry.id = id->Value();
		}
		entry.path = relative(owner.IsAlive() ? owner : root, entity);
		const i32 i = static_cast<i32>(saved.members.size());
		if (entity == selection.Active()) {
			saved.active = i;
		}
		if (entity == selection.Anchor()) {
			saved.anchor = i;
		}
		saved.members.push_back(std::move(entry));
	}
	return saved;
}

EntitySelection SelectionBookmark::Restore(Entity root,
	const std::unordered_map<u64, Entity> &index) const {
	std::vector<Entity> restored;
	Entity activeEntity, anchorEntity;
	for (std::size_t i = 0; i < members.size(); ++i) {
		const auto &entry = members[i];
		Entity entity;
		const auto found = index.find(entry.id);
		if (found != index.end()) {
			entity = found->second;
			if (!entry.path.empty()) {
				entity = entity.Find(entry.path);
			}
		}
		if (!belongs(entity, root)) {
			continue;
		}
		restored.push_back(entity);
		if (static_cast<i32>(i) == active) {
			activeEntity = entity;
		}
		if (static_cast<i32>(i) == anchor) {
			anchorEntity = entity;
		}
	}
	EntitySelection result;
	result.Assign(restored, activeEntity, anchorEntity);
	return result;
}
} // namespace sowa::editor
