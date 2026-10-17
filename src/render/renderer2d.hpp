#pragma once

#include <map>
#include <string_view>
#include <vector>

#include <glm/glm.hpp>

#include "core/types.hpp"
#include "math/color.hpp"
#include "math/rect.hpp"
#include "math/vector2.hpp"
#include "particles/particles2d.hpp"
#include "render/graphics_device.hpp"

namespace sowa {

class FileSystem;

// Batched 2D quad renderer with multiple texture slots.
class Renderer2D {
  public:
	Renderer2D() = default;
	~Renderer2D();

	Renderer2D(const Renderer2D &) = delete;
	Renderer2D &operator=(const Renderer2D &) = delete;

	// Empty shaderPath selects the embedded shader.
	bool Init(GraphicsDevice &device, FileSystem &fs, std::string_view shaderPath = {});
	void Shutdown();

	bool IsInitialized() const {
		return _device != nullptr;
	}

	// Plain ortho with the origin at the bottom-left - the no-camera framing.
	void SetProjection(f32 width, f32 height);
	// Transforms world-space vertices. Set before Begin to apply it to the batch.
	void SetViewProjection(const glm::mat4 &viewProjection);

	const glm::mat4 &ViewProjection() const {
		return _projection;
	}

	void Begin();
	// Clips are in the same coordinates as submitted vertices; changes stay batched.
	void PushClip(const Rect &rect);
	void PopClip();
	Rect CurrentClip() const;
	void SetBlendMode(ParticleBlendMode mode);
	// Flushes on a change so soft effects can fade below the sprite pick cutoff.
	void SetAlphaCutoff(f32 cutoff);
	TextureHandle ParticleTexture(ParticleDrawShape shape) const;
	// Solid-color quad (uses an internal 1x1 white texture).
	void DrawQuad(f32 x, f32 y, f32 w, f32 h, const Color &color, f32 z = 0.f, u32 entityToken = 0);
	// Textured quad; color tints the sample (default white = no tint).
	void DrawQuad(f32 x, f32 y, f32 w, f32 h, TextureHandle texture, const Color &color = Color{},
		f32 z = 0.f, u32 entityToken = 0);
	// Transforms localRect, including rotation and non-uniform scale.
	void DrawQuad(const glm::mat4 &transform, const Rect &localRect, TextureHandle texture,
		const Color &color = Color{}, f32 z = 0.f, u32 entityToken = 0);
	void DrawQuad(const glm::mat4 &transform, const Rect &localRect, const Rect &uvRect,
		TextureHandle texture, const Color &color = Color{}, f32 z = 0.f, u32 entityToken = 0);
	// Axis-aligned quad sampling a texture region, for atlases and glyphs.
	void DrawQuad(f32 x, f32 y, f32 w, f32 h, const Rect &uvRect, TextureHandle texture,
		const Color &color = Color{}, f32 z = 0.f, u32 entityToken = 0);
	// Debug or gizmo segment drawn as a thin quad in the sprite batch. Thickness is in world
	// pixels.
	void DrawLine(Vector2 a, Vector2 b, const Color &color, f32 thickness = 2.f, f32 z = 0.f);
	void End();

	// Submission statistics since ResetStats, accumulated across Begin/End pairs.
	struct Stats {
		// One per batch flush; each flush is one DrawIndexed.
		u32 drawCalls = 0;
		u32 quads = 0;
		// Textures bound for batches, summed over draw calls (the white
		// texture filling unused sampler units is not counted).
		u32 textureBinds = 0;
	};

	const Stats &GetStats() const {
		return _stats;
	}

	void ResetStats() {
		_stats = {};
	}

  private:
	struct Vertex {
		f32 x = 0.f;
		f32 y = 0.f;
		f32 z = 0.f;
		f32 u = 0.f;
		f32 v = 0.f;
		f32 r = 1.f;
		f32 g = 1.f;
		f32 b = 1.f;
		f32 a = 1.f;
		f32 textureIndex = 0.f;
		u32 entityToken = 0;
		f32 clipLeft = -1e20f, clipBottom = -1e20f;
		f32 clipRight = 1e20f, clipTop = 1e20f;
	};

	struct TextureHandleLess {
		bool operator()(const TextureHandle &a, const TextureHandle &b) const {
			if (a.id != b.id) {
				return a.id < b.id;
			}
			return a.version < b.version;
		}
	};

	static constexpr u32 MaxQuads = 1000;
	static constexpr u32 MaxVertices = MaxQuads * 4;
	static constexpr u32 MaxIndices = MaxQuads * 6;
	static constexpr u32 MaxTextures = 16;

	void pushQuad(f32 x, f32 y, f32 w, f32 h, const Rect &uvRect, f32 z, const Color &color,
		TextureHandle texture, u32 entityToken);
	void pushQuad(f32 x, f32 y, f32 w, f32 h, f32 z, const Color &color, TextureHandle texture,
		u32 entityToken);
	void pushQuad(const glm::mat4 &transform, const Rect &localRect, f32 z, const Color &color,
		TextureHandle texture, u32 entityToken, const Rect &uvRect = Rect(0, 0, 1, 1));
	u32 getTextureSlot(TextureHandle texture);
	void flush();
	void submitVertices(Vertex (&vertices)[4]);
	std::vector<Rect> _clips;

	GraphicsDevice *_device = nullptr;
	ShaderHandle _shader;
	PipelineHandle _pipeline;
	PipelineHandle _additivePipeline;
	ParticleBlendMode _blendMode = ParticleBlendMode::Alpha;
	f32 _alphaCutoff = 0.1f;
	TextureHandle _softParticleTexture, _circleParticleTexture;
	BufferHandle _vertexBuffer;
	BufferHandle _indexBuffer;
	TextureHandle _whiteTexture;

	glm::mat4 _projection{1.f};
	glm::mat4 _view{1.f};
	std::vector<Vertex> _vertices;
	std::map<TextureHandle, u32, TextureHandleLess> _batchTextures;
	Stats _stats;
};

} // namespace sowa
