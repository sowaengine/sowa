#pragma once

#include "core/reflection/field_type.hpp"
#include "core/types.hpp"
#include "ecs/entity.hpp"

namespace sowa {

// How an entity's z-index relates to its parent's.
enum class ZIndexMode {
	// Adds the nearest ancestor's effective z-index.
	Relative,
	// Ignores ancestor z-index and supplies the base for relative descendants.
	Absolute,
};

SOWA_REFLECT_ENUM(ZIndexMode, {"Relative", ZIndexMode::Relative},
	{"Absolute", ZIndexMode::Absolute});

// How an entity's render layer relates to its parent's.
enum class RenderLayerMode {
	// Use the nearest ancestor's effective layer; this entity's own layer
	// value is ignored. The default, so adding a RenderOrder purely to set a
	// z-index can't silently pull something out of its parent's layer.
	Inherit,
	// Use this entity's layer outright, and hand it down to descendants that
	// inherit. This is what puts a whole UI subtree above the game world no
	// matter what z-indices are flying around below it.
	Explicit,
};

SOWA_REFLECT_ENUM(RenderLayerMode, {"Inherit", RenderLayerMode::Inherit},
	{"Explicit", RenderLayerMode::Explicit});

// Draw key is (effectiveLayer, effectiveZIndex). Layer and z-index inherit independently;
// explicit layer plus Absolute z-index isolates a subtree. Defaults to (0, 0).
struct RenderOrder {
	i32 zIndex = 0;
	ZIndexMode zIndexMode = ZIndexMode::Relative;
	i32 layer = 0;
	RenderLayerMode layerMode = RenderLayerMode::Inherit;

	// Derived by CalculateRenderOrder; excluded from reflection.
	i32 effectiveZIndex = 0;
	i32 effectiveLayer = 0;
};

// Shared draw-order key. Layer takes precedence over z-index.
struct RenderSortKey {
	i32 layer = 0;
	i32 zIndex = 0;

	bool operator<(const RenderSortKey &other) const {
		if (layer != other.layer) {
			return layer < other.layer;
		}
		return zIndex < other.zIndex;
	}
};

// Reads derived draw order after CalculateRenderOrder. A null component gives (0, 0).
RenderSortKey GetRenderSortKey(const RenderOrder *order);

// Computes effective z-index from current ancestors in O(depth), before cached values
// are updated. Ancestors without RenderOrder are skipped.
i32 ComputeEffectiveZIndex(Entity entity);

// Computes the nearest explicit ancestor layer, including this entity.
// Returns zero if unset; use when cached ordering is not current.
i32 ComputeEffectiveLayer(Entity entity);

} // namespace sowa
