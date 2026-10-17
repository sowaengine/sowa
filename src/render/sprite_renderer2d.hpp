#pragma once

#include <optional>
#include <vector>

#include <glm/glm.hpp>

#include "core/types.hpp"
#include "ecs/components/animated_sprite.hpp"
#include "ecs/components/nine_patch_rect.hpp"
#include "ecs/components/render_order.hpp"
#include "ecs/components/sprite.hpp"
#include "ecs/components/text2d.hpp"
#include "ecs/components/tilemap.hpp"
#include "ecs/components/transform2d.hpp"
#include "ecs/entity.hpp"
#include "ecs/entity_pick_map.hpp"
#include "ecs/query.hpp"
#include "math/color.hpp"
#include "math/rect.hpp"
#include "particles/particles2d.hpp"
#include "render/graphics_device.hpp"
#include "resource/resource_registry.hpp"

namespace sowa {

class Renderer2D;
class World;

// Collects, sorts, and draws visible Sprite entities through Renderer2D.
// Owned alongside the renderer; collection and sorting are separate from per-entity ECS
// systems.
class SpriteRenderer2D {
  public:
	// Registers Sprite and caches its query once per World. World and registry must outlive the
	// renderer.
	void Init(World &world, ResourceRegistry &resources);

	void SetDefaultFont(ResourceID font) {
		_defaultFont = font;
	}

	// Collects, sorts, and submits visible sprites in one Begin/End batch after Progress.
	// cameraPosition includes offset for parallax; projection determines tile coverage.
	void Draw(Renderer2D &renderer, EntityPickMap &picks, Vector2 cameraPosition = {});

	struct DrawItem {
		RenderSortKey key;
		Entity entity;
		// Tie-breaks equal keys so same-order sprites sharing a texture end up
		// adjacent. Order among equal keys is unspecified anyway, and grouping
		// by texture means fewer batch flushes.
		u32 textureId = 0;
		glm::mat4 transform{1.f};
		Rect localRect;
		Color modulate;
		TextureHandle texture;
		Rect uvRect{0, 0, 1, 1};
		ParticleBlendMode blendMode = ParticleBlendMode::Alpha;
		f32 alphaCutoff = 0.1f;
		bool proceduralParticle = false;
		ParticleDrawShape particleShape = ParticleDrawShape::Quad;
		std::optional<Rect> clip;
	};

	// Rebuilds the sorted draw list without GPU submission.
	// Use the rendering projection for viewport-dependent tiling.
	void Collect(const glm::mat4 &viewProjection = glm::mat4(1.f), Vector2 cameraPosition = {});

	// The last collected list, in submission order.
	const std::vector<DrawItem> &DrawList() const {
		return _drawList;
	}

  private:
	void applyParallax(const glm::mat4 &viewProjection, Vector2 cameraPosition);
	// VisibleOnly, so hidden entities and their subtrees never reach the sort.
	Query<Transform2D, Sprite> _sprites;
	Query<Transform2D, NinePatchRect> _ninePatches;
	Query<Transform2D, AnimatedSprite> _animatedSprites;
	Query<Transform2D, Tilemap> _tilemaps;
	Query<Transform2D, Text2D> _texts;
	Query<Transform2D, Particles2D> _particles;
	ResourceRegistry *_resources = nullptr;
	ResourceID _defaultFont;
	std::vector<DrawItem> _drawList;
};

} // namespace sowa
