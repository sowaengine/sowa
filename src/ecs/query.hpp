#pragma once

#include <utility>

#include <flecs.h>

#include "core/types.hpp"
#include "ecs/entity.hpp"

namespace sowa {

// Cached query created by World::CreateQuery<Components...>(). Reuse for per-frame
// work; World::Each builds a temporary query. Copies share the refcounted query.
// Must not outlive its World.
template <typename... Components> class Query {
  public:
	// A default-constructed Query is invalid; only Query objects from
	// World::CreateQuery can be iterated.
	Query() = default;

	explicit Query(flecs::query<Components...> query) : _query(std::move(query)) {
	}

	bool IsValid() const {
		return static_cast<bool>(_query);
	}

	// Calls func(Entity, Components&...) for each match. No-op for an invalid query.
	// Structural changes inside the callback can invalidate tables and make flecs abort.
	// Use QueueDestroy, or collect entities and destroy them after iteration.
	template <typename Func> void Each(Func &&func) {
		if (!IsValid()) {
			return;
		}
		_query.each([&](flecs::entity entity, Components &...components) {
			func(Entity(entity), components...);
		});
	}

	// Number of matching entities. 0 on an invalid Query.
	i32 Count() const {
		return IsValid() ? static_cast<i32>(_query.count()) : 0;
	}

	// Whether anything matches. Cheaper than Count() > 0: it stops at the
	// first match instead of walking every table.
	bool Any() const {
		return IsValid() && _query.is_true();
	}

	// First matching entity, or a null Entity if nothing matches. Useful for
	// the "there should be exactly one of these" cases (an active camera, a
	// player) without materializing the whole match set.
	Entity First() const {
		return IsValid() ? Entity(_query.first()) : Entity();
	}

  private:
	flecs::query<Components...> _query;
};

} // namespace sowa
