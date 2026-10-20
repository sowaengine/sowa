#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "ecs/entity.hpp"

namespace sowa::editor {

enum class SelectionMode {
	Replace,
	Add,
	Toggle
};

class EntitySelection {
  public:
	const std::vector<Entity> &Members() const {
		return _members;
	}

	Entity Active() const {
		return _active;
	}

	Entity ActiveRoot() const;

	Entity Anchor() const {
		return _anchor;
	}

	bool Contains(Entity entity) const;
	void Clear();
	void Set(Entity entity);
	void Assign(std::vector<Entity> entities, Entity active = {}, Entity anchor = {});
	void Click(Entity entity, SelectionMode mode = SelectionMode::Replace);
	void Range(Entity entity, const std::vector<Entity> &visible, bool additive);
	void Prune(Entity root = {});
	void Activate(Entity entity);
	bool operator==(const EntitySelection &) const = default;

  private:
	std::vector<Entity> _members;
	Entity _active, _anchor;
};

std::vector<Entity> SelectionRoots(const std::vector<Entity> &members);

struct SelectionBookmark {
	struct Entry {
		u64 id = 0;
		std::string path;
	};

	std::vector<Entry> members;
	i32 active = -1, anchor = -1;
	static SelectionBookmark Capture(Entity root, const EntitySelection &selection);
	EntitySelection Restore(Entity root, const std::unordered_map<u64, Entity> &index) const;
};

} // namespace sowa::editor
