#include "ecs/components/render_order.hpp"

#include <vector>

namespace sowa {

RenderSortKey GetRenderSortKey(const RenderOrder *order) {
	if (order == nullptr) {
		return RenderSortKey{};
	}
	return RenderSortKey{order->effectiveLayer, order->effectiveZIndex};
}

i32 ComputeEffectiveZIndex(Entity entity) {
	// Collected child-first, then folded root-first, matching the order the
	// cascade pass produces so both paths agree.
	std::vector<const RenderOrder *> chain;
	for (Entity current = entity; current.IsAlive(); current = current.GetParent()) {
		if (const RenderOrder *order = current.Get<RenderOrder>()) {
			chain.push_back(order);
			// An Absolute entry ignores everything above it, so there is no
			// point walking further up.
			if (order->zIndexMode == ZIndexMode::Absolute) {
				break;
			}
		}
	}

	i32 effective = 0;
	for (auto it = chain.rbegin(); it != chain.rend(); ++it) {
		const RenderOrder &order = **it;
		effective =
			order.zIndexMode == ZIndexMode::Absolute ? order.zIndex : effective + order.zIndex;
	}
	return effective;
}

i32 ComputeEffectiveLayer(Entity entity) {
	// No accumulation to do: the first explicit layer at or above this entity
	// is the answer, since everything between inherits it unchanged.
	for (Entity current = entity; current.IsAlive(); current = current.GetParent()) {
		const RenderOrder *order = current.Get<RenderOrder>();
		if (order != nullptr && order->layerMode == RenderLayerMode::Explicit) {
			return order->layer;
		}
	}
	return 0;
}

} // namespace sowa
