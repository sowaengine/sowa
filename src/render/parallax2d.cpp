#include "render/sprite_renderer2d.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <map>

#include "ecs/components/parallax2d.hpp"

namespace sowa {
namespace {

using DrawItem = SpriteRenderer2D::DrawItem;

struct Bounds {
	glm::vec2 min{std::numeric_limits<f32>::infinity()};
	glm::vec2 max{-std::numeric_limits<f32>::infinity()};

	void Include(const glm::mat4 &transform, const Rect &rect) {
		for (float x : {rect.x, rect.Right()}) {
			for (float y : {rect.y, rect.Top()}) {
				const glm::vec2 point = transform * glm::vec4(x, y, 0, 1);
				min = glm::min(min, point);
				max = glm::max(max, point);
			}
		}
	}

	Rect AsRect() const {
		return {min.x, min.y, max.x - min.x, max.y - min.y};
	}
};

struct Layer {
	Entity entity;
	std::vector<DrawItem> items;
};

Entity FindLayer(Entity entity) {
	while (entity.IsAlive() && !entity.Has<Parallax2D>()) {
		entity = entity.GetParent();
	}
	return entity;
}

bool IsInvertible(const glm::mat4 &matrix) {
	const float determinant = glm::determinant(matrix);
	return std::isfinite(determinant) && determinant != 0;
}

Bounds VisibleArea(Bounds view, const Parallax2D &settings) {
	if (settings.limitX) {
		view.min.x = std::max(view.min.x, settings.limitMin.x);
		view.max.x = std::min(view.max.x, settings.limitMax.x);
	}
	if (settings.limitY) {
		view.min.y = std::max(view.min.y, settings.limitMin.y);
		view.max.y = std::min(view.max.y, settings.limitMax.y);
	}
	return view;
}

void RepeatLayer(const Layer &layer, const Bounds &worldView, Vector2 camera,
	std::vector<DrawItem> &output) {
	const auto &settings = *layer.entity.Get<Parallax2D>();
	const Bounds area = VisibleArea(worldView, settings);
	if (area.min.x >= area.max.x || area.min.y >= area.max.y) {
		return;
	}

	const auto world = ComputeWorldMatrix(layer.entity);
	if (!IsInvertible(world)) {
		return;
	}

	const auto inverse = glm::inverse(world);
	Bounds artwork;
	for (const auto &item : layer.items) {
		artwork.Include(inverse * item.transform, item.localRect);
	}
	const glm::vec2 tileSize = artwork.max - artwork.min;
	if (!std::isfinite(tileSize.x) || !std::isfinite(tileSize.y)) {
		return;
	}

	const Vector2 speed{settings.xSpeed, settings.ySpeed};
	const Vector2 offset = camera * (Vector2(1) - speed) + settings.offset;
	const glm::vec4 translation{offset.x, offset.y, 0, 0};
	if (!std::isfinite(offset.x) || !std::isfinite(offset.y)) {
		return;
	}

	auto shiftedWorld = world;
	shiftedWorld[3] += translation;
	Bounds localView;
	localView.Include(glm::inverse(shiftedWorld), area.AsRect());

	glm::dvec2 first{0};
	glm::dvec2 last{0};
	const bool tiled[] = {settings.tileX, settings.tileY};
	for (int axis = 0; axis < 2; ++axis) {
		if (tiled[axis] && tileSize[axis] > 0) {
			const double start = double(localView.min[axis]) - artwork.max[axis];
			const double end = double(localView.max[axis]) - artwork.min[axis];
			first[axis] = std::floor(start / tileSize[axis]) + 1;
			last[axis] = std::ceil(end / tileSize[axis]) - 1;
		}
	}

	if (first.x > last.x || first.y > last.y) {
		return;
	}

	// Bound the work for degenerate artwork or extreme camera zoom.
	const glm::dvec2 count = last - first + glm::dvec2(1);
	const double copies = count.x * count.y;
	if (!std::isfinite(copies) || copies > 4096) {
		return;
	}

	const int columns = static_cast<int>(count.x);
	const int rows = static_cast<int>(count.y);
	for (int row = 0; row < rows; ++row) {
		for (int column = 0; column < columns; ++column) {
			const float x = float((first.x + column) * tileSize.x);
			const float y = float((first.y + row) * tileSize.y);
			const glm::vec4 step{x, y, 0, 0};
			for (auto item : layer.items) {
				item.transform[3] += translation + world * step;
				if (settings.limitX || settings.limitY) {
					item.clip = area.AsRect();
				}
				output.push_back(item);
			}
		}
	}
}

} // namespace

void SpriteRenderer2D::applyParallax(const glm::mat4 &viewProjection, Vector2 cameraPosition) {
	if (!IsInvertible(viewProjection)) {
		return;
	}

	Bounds worldView;
	worldView.Include(glm::inverse(viewProjection), {-1, -1, 2, 2});

	std::map<flecs::entity_t, Layer> layers;
	std::vector<DrawItem> output;
	output.reserve(_drawList.size());

	for (const auto &item : _drawList) {
		const Entity owner = FindLayer(item.entity);
		if (owner.IsAlive()) {
			auto &layer = layers[owner.Raw().id()];
			layer.entity = owner;
			layer.items.push_back(item);
		} else {
			output.push_back(item);
		}
	}

	for (const auto &[id, layer] : layers) {
		RepeatLayer(layer, worldView, cameraPosition, output);
	}
	_drawList = std::move(output);
}

} // namespace sowa
