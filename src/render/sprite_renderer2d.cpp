#include "render/sprite_renderer2d.hpp"

#include "animation/animated_sprite.hpp"
#include "core/scope_exit.hpp"
#include "resource/sprite_sheet_animation.hpp"
#include "scene/tilemap_scenes.hpp"
#include <algorithm>

#include "ecs/entity.hpp"
#include "ecs/visibility.hpp"
#include "ecs/world.hpp"
#include "render/image_patches.hpp"
#include "render/renderer2d.hpp"
#include "resource/font.hpp"
#include "resource/image_texture.hpp"

namespace sowa {

void SpriteRenderer2D::Init(World &world, ResourceRegistry &resources) {
	_resources = &resources;
	_ninePatches = world.CreateQuery<Transform2D, NinePatchRect>(VisibilityFilter::VisibleOnly);
	_texts = world.CreateQuery<Transform2D, Text2D>(VisibilityFilter::VisibleOnly);
	RegisterAnimatedSpriteSystem(world, resources);
	_animatedSprites =
		world.CreateQuery<Transform2D, AnimatedSprite>(VisibilityFilter::VisibleOnly);
	_particles = world.CreateQuery<Transform2D, Particles2D>(VisibilityFilter::VisibleOnly);
	if (!world.Flecs().lookup("SyncTilemapScenes")) {
		world.Flecs()
			.system("SyncTilemapScenes")
			.kind(flecs::PreUpdate)
			.run([&world, &resources](flecs::iter &) {
				world.SuspendDefer();
				auto resume = MakeScopeExit([&world] { world.ResumeDefer(); });
				SyncTilemapScenes(world, resources);
			});
	}
	_tilemaps = world.CreateQuery<Transform2D, Tilemap>(VisibilityFilter::VisibleOnly);

	world.RegisterComponent<Sprite>("Sprite")
		.ResourceField("texture", &Sprite::texture, "ImageTexture")
		.Field("modulate", &Sprite::modulate)
		.Field("size", &Sprite::size)
		.Field("offset", &Sprite::offset)
		.Field("centered", &Sprite::centered);

	_sprites = world.CreateQuery<Transform2D, Sprite>(VisibilityFilter::VisibleOnly);
}

void SpriteRenderer2D::Draw(Renderer2D &renderer, EntityPickMap &picks, Vector2 cameraPosition) {
	Collect(renderer.ViewProjection(), cameraPosition);

	renderer.Begin();
	for (const DrawItem &item : _drawList) {
		renderer.SetBlendMode(item.blendMode);
		renderer.SetAlphaCutoff(item.alphaCutoff);
		const auto texture =
			item.proceduralParticle ? renderer.ParticleTexture(item.particleShape) : item.texture;
		if (item.clip) {
			renderer.PushClip(*item.clip);
		}
		renderer.DrawQuad(item.transform, item.localRect, item.uvRect, texture, item.modulate, 0.f,
			picks.TokenFor(item.entity));
		if (item.clip) {
			renderer.PopClip();
		}
	}
	renderer.End();
}

void SpriteRenderer2D::Collect(const glm::mat4 &viewProjection, Vector2 cameraPosition) {
	_drawList.clear();
	if (!_sprites.IsValid() || _resources == nullptr) {
		return;
	}

	_sprites.Each([&](Entity entity, Transform2D &transform, Sprite &sprite) {
		// Resolve each frame so resource reloads cannot leave cached graphics handles stale.
		const Ref<ImageTexture> texture = _resources->Get<ImageTexture>(sprite.texture);
		if (texture == nullptr || !texture->IsLoaded()) {
			return;
		}

		// A zero-size sprite would submit a degenerate quad; skipping keeps it
		// out of the batch entirely. With size defaulting to the texture's own
		// dimensions this now only catches a genuinely empty image.
		const Vector2 size = sprite.ResolvedSize(texture->Size());
		if (size.x == 0.f || size.y == 0.f) {
			return;
		}

		const TextureHandle handle = texture->Handle();
		_drawList.push_back(DrawItem{GetRenderSortKey(entity.Get<RenderOrder>()), entity, handle.id,
			transform.worldMatrix, sprite.LocalRect(size), sprite.modulate, handle});
	});

	_ninePatches.Each([&](Entity entity, Transform2D &transform, NinePatchRect &image) {
		const auto texture = _resources->Get<ImageTexture>(image.texture);
		if (!texture || !texture->IsLoaded() || image.modulate.a <= 0) {
			return;
		}
		const ImagePatchOptions options{
			.paddingLeft = image.paddingLeft,
			.paddingRight = image.paddingRight,
			.paddingTop = image.paddingTop,
			.paddingBottom = image.paddingBottom,
			.ninePatch = true,
			.tileCenter = image.tileCenter,
			.tileEdges = image.tileEdges,
			.drawCenter = image.drawCenter,
		};
		const Rect local = image.LocalRect();
		const auto handle = texture->Handle();
		const RenderSortKey key{ComputeEffectiveLayer(entity), ComputeEffectiveZIndex(entity)};
		for (const auto &patch : BuildImagePatches({0, 0, local.w, local.h}, texture->Size(),
				 texture->flipVertically, options)) {
			const Rect rect{local.x + patch.rect.x, local.Top() - patch.rect.y - patch.rect.h,
				patch.rect.w, patch.rect.h};
			_drawList.push_back({key, entity, handle.id, transform.worldMatrix, rect,
				image.modulate, handle, patch.uv});
		}
	});

	_animatedSprites.Each([&](Entity entity, Transform2D &transform, AnimatedSprite &sprite) {
		const auto library = _resources->Get<SpriteSheetAnimation>(sprite.animations);
		const auto *clip = library ? library->Find(sprite.currentAnimation) : nullptr;
		if (!clip || !clip->IsValid()) {
			return;
		}
		const auto texture = _resources->Get<ImageTexture>(clip->texture);
		if (!texture || !texture->IsLoaded()) {
			return;
		}
		const i32 frame = std::clamp(sprite.frame, 0, clip->FrameCount() - 1);
		const Rect uv = clip->FrameUV(frame, texture->Size(), texture->flipVertically);
		if (uv.w <= 0 || uv.h == 0) {
			return;
		}
		const Rect cell = clip->CellRect(clip->frames[frame], texture->Size());
		const Rect rect = sprite.LocalRect({cell.w, cell.h});
		if (rect.w == 0 || rect.h == 0) {
			return;
		}
		const auto handle = texture->Handle();
		_drawList.push_back({GetRenderSortKey(entity.Get<RenderOrder>()), entity, handle.id,
			transform.worldMatrix, rect, sprite.modulate, handle, uv});
	});

	_texts.Each([&](Entity entity, Transform2D &transform, Text2D &text) {
		const auto font = _resources->Get<Font>(text.font ? text.font : _defaultFont);
		if (!font || !font->IsLoaded() || text.modulate.a <= 0.f) {
			return;
		}
		const auto layout = LayoutTextBlock(*font, text.text, text.LayoutOptions());
		const auto handle = font->Atlas();
		const RenderSortKey key{ComputeEffectiveLayer(entity), ComputeEffectiveZIndex(entity)};
		for (const auto &quad : layout.quads) {
			DrawItem item{key, entity, handle.id, transform.worldMatrix, quad.rect, text.modulate,
				handle, quad.uvRect};
			item.alphaCutoff = 0.001f;
			_drawList.push_back(item);
		}
	});

	_tilemaps.Each([&](Entity entity, Transform2D &transform, Tilemap &map) {
		for (const auto &sheet : map.Sheets()) {
			if (const auto texture = _resources->Get<ImageTexture>(sheet.resource)) {
				map.RefreshSheet(sheet.id, texture->Size());
			}
		}
		if (!map.ValidCellSize()) {
			return;
		}
		// Resolve once per palette entry, rather than once per occupied cell.
		struct Resolved {
			TextureHandle handle;
			Rect uv;
		};
		std::map<i32, Resolved> textures;
		for (const auto &tile : map.Tiles()) {
			if (tile.scene) {
				continue;
			}
			const auto texture = _resources->Get<ImageTexture>(tile.resource);
			if (!texture || !texture->IsLoaded()) {
				continue;
			}
			const Rect uv = TileUV(tile, texture->Size(), texture->flipVertically);
			if (uv.w == 0 || uv.h == 0) {
				continue;
			}
			textures.emplace(tile.id, Resolved{texture->Handle(), uv});
		}
		for (i32 index = 0; index < static_cast<i32>(map.Layers().size()); ++index) {
			const auto &layer = map.Layers()[index];
			if (!layer.visible) {
				continue;
			}
			RenderSortKey key{ComputeEffectiveLayer(entity),
				ComputeEffectiveZIndex(entity) + index};
			for (const auto &[cell, id] : layer.cells) {
				const auto it = textures.find(id);
				if (it == textures.end()) {
					continue;
				}
				const auto &texture = it->second;
				_drawList.push_back({key, entity, texture.handle.id, transform.worldMatrix,
					Rect(float(cell.x) * map.cellSize.x, float(cell.y) * map.cellSize.y,
						map.cellSize.x, map.cellSize.y),
					Color{}, texture.handle, texture.uv});
			}
		}
	});

	_particles.Each([&](Entity entity, Transform2D &transform, Particles2D &emitter) {
		const auto texture = _resources->Get<ImageTexture>(emitter.texture);
		const bool textured = texture && texture->IsLoaded();
		// A missing/reloading texture still has a useful procedural preview.
		const auto handle = textured ? texture->Handle() : TextureHandle{};
		const RenderSortKey key{ComputeEffectiveLayer(entity), ComputeEffectiveZIndex(entity)};
		for (const auto &particle : emitter.runtime.particles) {
			const f32 size = ParticleSize(emitter, particle);
			const Color color = ParticleColor(emitter, particle);
			if (size <= 0 || color.a <= 0) {
				continue;
			}
			DrawItem item{key, entity, handle.id,
				ParticleTransform(emitter, particle, transform.worldMatrix),
				Rect(-size / 2, -size / 2, size, size), color, handle};
			if (textured) {
				item.uvRect = ParticleUV(emitter, particle, texture->flipVertically);
			}
			item.blendMode = emitter.blendMode;
			item.alphaCutoff = 0.001f;
			item.proceduralParticle = !textured;
			item.particleShape = emitter.drawShape;
			_drawList.push_back(item);
		}
	});

	applyParallax(viewProjection, cameraPosition);

	// Submission order determines alpha-blended draw order. Batch flushes preserve it.
	std::stable_sort(_drawList.begin(), _drawList.end(), [](const DrawItem &a, const DrawItem &b) {
		if (a.key < b.key) {
			return true;
		}
		if (b.key < a.key) {
			return false;
		}
		return a.textureId < b.textureId;
	});
}

} // namespace sowa
