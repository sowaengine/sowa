#include "render/renderer2d.hpp"
#include <array>
#include <cmath>

#include "assets/embedded.hpp"
#include "core/log.hpp"
#include "math/matrix.hpp"
#include "render/shader.hpp"

namespace sowa {

Renderer2D::~Renderer2D() {
	Shutdown();
}

bool Renderer2D::Init(GraphicsDevice &device, FileSystem &fs, std::string_view shaderPath) {
	Shutdown();

	_device = &device;

	if (shaderPath.empty()) {
		const auto source = assets::SpriteShader();
		_shader = CreateShaderFromSource(device,
			std::string_view(reinterpret_cast<const char *>(source.data()), source.size()));
	} else {
		_shader = CreateShaderFromFile(device, fs, shaderPath);
	}
	if (!_shader.IsValid()) {
		Log::Error("Renderer2D: failed to load shader {}", shaderPath);
		Shutdown();
		return false;
	}

	PipelineDesc pipelineDesc;
	pipelineDesc.shader = _shader;
	pipelineDesc.alphaBlend = true;
	pipelineDesc.layout.attributes = {
		{0, VertexAttrType::Vec3},	 // position
		{1, VertexAttrType::Vec2},	 // uv
		{2, VertexAttrType::Vec4},	 // color
		{3, VertexAttrType::Float},	 // texture index
		{4, VertexAttrType::UInt32}, // entity-picking token
		{5, VertexAttrType::Vec4},	 // effective logical clip
	};
	_pipeline = device.CreatePipeline(pipelineDesc);
	if (!_pipeline.IsValid()) {
		Log::Error("Renderer2D: failed to create pipeline");
		Shutdown();
		return false;
	}

	pipelineDesc.additiveBlend = true;
	_additivePipeline = device.CreatePipeline(pipelineDesc);
	if (!_additivePipeline.IsValid()) {
		Shutdown();
		return false;
	}

	_vertexBuffer = device.CreateBuffer(BufferType::Vertex, BufferUsage::Dynamic,
		MaxVertices * static_cast<u32>(sizeof(Vertex)));
	if (!_vertexBuffer.IsValid()) {
		Log::Error("Renderer2D: failed to create vertex buffer");
		Shutdown();
		return false;
	}

	std::vector<u32> indices(MaxIndices);
	u32 offset = 0;
	for (u32 i = 0; i < MaxQuads; ++i) {
		const u32 base = i * 4;
		indices[offset++] = base + 0;
		indices[offset++] = base + 1;
		indices[offset++] = base + 2;
		indices[offset++] = base + 0;
		indices[offset++] = base + 2;
		indices[offset++] = base + 3;
	}
	_indexBuffer = device.CreateBuffer(BufferType::Index, BufferUsage::Static,
		static_cast<u32>(indices.size() * sizeof(u32)), indices.data());
	if (!_indexBuffer.IsValid()) {
		Log::Error("Renderer2D: failed to create index buffer");
		Shutdown();
		return false;
	}

	const u8 whitePixel[4] = {255, 255, 255, 255};
	_whiteTexture = device.CreateTexture(1, 1, whitePixel);
	if (!_whiteTexture.IsValid()) {
		Log::Error("Renderer2D: failed to create white texture");
		Shutdown();
		return false;
	}

	// Radial masks are renderer-owned and generated once, shared by emitters.
	std::array<u8, 64 * 64 * 4> pixels{};
	for (bool soft : {true, false}) {
		for (i32 y = 0; y < 64; ++y) {
			for (i32 x = 0; x < 64; ++x) {
				const f32 dx = (x + 0.5f - 32) / 32, dy = (y + 0.5f - 32) / 32;
				const f32 radius = std::sqrt(dx * dx + dy * dy);
				const f32 alpha = soft ? std::pow(std::max(0.f, 1 - radius * radius), 2.f)
									   : std::clamp((1 - radius) * 32, 0.f, 1.f);
				const i32 index = (y * 64 + x) * 4;
				pixels[index] = pixels[index + 1] = pixels[index + 2] = 255;
				pixels[index + 3] = static_cast<u8>(alpha * 255);
			}
		}
		auto &handle = soft ? _softParticleTexture : _circleParticleTexture;
		handle = device.CreateTexture(64, 64, pixels.data());
		if (!handle.IsValid()) {
			Shutdown();
			return false;
		}
	}
	_vertices.reserve(MaxVertices);
	_view = glm::mat4(1.f);
	return true;
}

void Renderer2D::Shutdown() {
	if (!_device) {
		return;
	}

	for (auto *handle : {&_softParticleTexture, &_circleParticleTexture}) {
		if (handle->IsValid()) {
			_device->DestroyTexture(*handle);
		}
		*handle = {};
	}
	if (_additivePipeline.IsValid()) {
		_device->DestroyPipeline(_additivePipeline);
		_additivePipeline = {};
	}
	if (_whiteTexture.IsValid()) {
		_device->DestroyTexture(_whiteTexture);
		_whiteTexture = {};
	}
	if (_indexBuffer.IsValid()) {
		_device->DestroyBuffer(_indexBuffer);
		_indexBuffer = {};
	}
	if (_vertexBuffer.IsValid()) {
		_device->DestroyBuffer(_vertexBuffer);
		_vertexBuffer = {};
	}
	if (_pipeline.IsValid()) {
		_device->DestroyPipeline(_pipeline);
		_pipeline = {};
	}
	if (_shader.IsValid()) {
		_device->DestroyShader(_shader);
		_shader = {};
	}

	_vertices.clear();
	_batchTextures.clear();
	_device = nullptr;
}

void Renderer2D::SetProjection(f32 width, f32 height) {
	_projection = Matrix::CalculateOrtho(0.f, width, 0.f, height);
}

void Renderer2D::SetViewProjection(const glm::mat4 &viewProjection) {
	_projection = viewProjection;
}

TextureHandle Renderer2D::ParticleTexture(ParticleDrawShape shape) const {
	if (shape == ParticleDrawShape::SoftCircle) {
		return _softParticleTexture;
	}
	if (shape == ParticleDrawShape::Circle) {
		return _circleParticleTexture;
	}
	return _whiteTexture;
}

void Renderer2D::SetBlendMode(ParticleBlendMode mode) {
	if (_blendMode == mode) {
		return;
	}
	flush();
	_blendMode = mode;
}

void Renderer2D::SetAlphaCutoff(f32 cutoff) {
	cutoff = std::isfinite(cutoff) ? std::clamp(cutoff, 0.f, 1.f) : 0.1f;
	if (_alphaCutoff == cutoff) {
		return;
	}
	flush();
	_alphaCutoff = cutoff;
}

void Renderer2D::Begin() {
	_clips.clear();
	_alphaCutoff = 0.1f;
	_blendMode = ParticleBlendMode::Alpha;
	_vertices.clear();
	_batchTextures.clear();
}

void Renderer2D::DrawQuad(f32 x, f32 y, f32 w, f32 h, const Color &color, f32 z, u32 entityToken) {
	pushQuad(x, y, w, h, z, color, _whiteTexture, entityToken);
}

void Renderer2D::DrawQuad(f32 x, f32 y, f32 w, f32 h, TextureHandle texture, const Color &color,
	f32 z, u32 entityToken) {
	pushQuad(x, y, w, h, z, color, texture.IsValid() ? texture : _whiteTexture, entityToken);
}

void Renderer2D::DrawQuad(const glm::mat4 &transform, const Rect &localRect, TextureHandle texture,
	const Color &color, f32 z, u32 entityToken) {
	pushQuad(transform, localRect, z, color, texture.IsValid() ? texture : _whiteTexture,
		entityToken);
}

void Renderer2D::DrawQuad(const glm::mat4 &transform, const Rect &localRect, const Rect &uvRect,
	TextureHandle texture, const Color &color, f32 z, u32 entityToken) {
	pushQuad(transform, localRect, z, color, texture.IsValid() ? texture : _whiteTexture,
		entityToken, uvRect);
}

void Renderer2D::DrawQuad(f32 x, f32 y, f32 w, f32 h, const Rect &uvRect, TextureHandle texture,
	const Color &color, f32 z, u32 entityToken) {
	pushQuad(x, y, w, h, uvRect, z, color, texture.IsValid() ? texture : _whiteTexture,
		entityToken);
}

void Renderer2D::DrawLine(Vector2 a, Vector2 b, const Color &color, f32 thickness, f32 z) {
	const Vector2 delta = b - a;
	const f32 length = delta.Length();
	if (length <= 0.f || thickness <= 0.f) {
		return;
	}

	const Vector2 n = Vector2(-delta.y, delta.x) * (thickness * 0.5f / length);
	if (_vertices.size() + 4 > MaxVertices) {
		flush();
	}

	const u32 slot = getTextureSlot(_whiteTexture);
	const f32 texIndex = static_cast<f32>(slot);
	Vertex verts[4] = {
		{a.x + n.x, a.y + n.y, z, 0.f, 1.f, color.r, color.g, color.b, color.a, texIndex, 0},
		{a.x - n.x, a.y - n.y, z, 0.f, 0.f, color.r, color.g, color.b, color.a, texIndex, 0},
		{b.x - n.x, b.y - n.y, z, 1.f, 0.f, color.r, color.g, color.b, color.a, texIndex, 0},
		{b.x + n.x, b.y + n.y, z, 1.f, 1.f, color.r, color.g, color.b, color.a, texIndex, 0},
	};
	submitVertices(verts);
}

void Renderer2D::End() {
	flush();
}

u32 Renderer2D::getTextureSlot(TextureHandle texture) {
	const auto it = _batchTextures.find(texture);
	if (it != _batchTextures.end()) {
		return it->second;
	}

	if (_batchTextures.size() >= MaxTextures) {
		flush();
	}

	const u32 slot = static_cast<u32>(_batchTextures.size());
	_batchTextures.emplace(texture, slot);
	return slot;
}

void Renderer2D::pushQuad(f32 x, f32 y, f32 w, f32 h, f32 z, const Color &color,
	TextureHandle texture, u32 entityToken) {
	pushQuad(x, y, w, h, Rect(0.f, 0.f, 1.f, 1.f), z, color, texture, entityToken);
}

void Renderer2D::pushQuad(f32 x, f32 y, f32 w, f32 h, const Rect &uvRect, f32 z, const Color &color,
	TextureHandle texture, u32 entityToken) {
	if (_vertices.size() + 4 > MaxVertices) {
		flush();
	}

	const u32 slot = getTextureSlot(texture);
	const f32 texIndex = static_cast<f32>(slot);

	// UV origin is bottom-left, matching quad positions.
	const f32 u0 = uvRect.Left();
	const f32 u1 = uvRect.Right();
	const f32 v0 = uvRect.Bottom();
	const f32 v1 = uvRect.Top();

	Vertex verts[4] = {
		{x, y + h, z, u0, v1, color.r, color.g, color.b, color.a, texIndex, entityToken},
		{x, y, z, u0, v0, color.r, color.g, color.b, color.a, texIndex, entityToken},
		{x + w, y, z, u1, v0, color.r, color.g, color.b, color.a, texIndex, entityToken},
		{x + w, y + h, z, u1, v1, color.r, color.g, color.b, color.a, texIndex, entityToken},
	};

	submitVertices(verts);
}

void Renderer2D::pushQuad(const glm::mat4 &transform, const Rect &localRect, f32 z,
	const Color &color, TextureHandle texture, u32 entityToken, const Rect &uvRect) {
	if (_vertices.size() + 4 > MaxVertices) {
		flush();
	}

	const u32 slot = getTextureSlot(texture);
	const f32 texIndex = static_cast<f32>(slot);

	// Same corner order and UV assignment as the axis-aligned path, so both
	// agree on which way up a texture is: bottom-left is (0,0), matching the
	// vertical flip CreateTextureFromFile does on load for Y-up.
	const glm::vec4 corners[4] = {
		{localRect.Left(), localRect.Top(), 0.f, 1.f},
		{localRect.Left(), localRect.Bottom(), 0.f, 1.f},
		{localRect.Right(), localRect.Bottom(), 0.f, 1.f},
		{localRect.Right(), localRect.Top(), 0.f, 1.f},
	};
	const f32 us[4] = {uvRect.Left(), uvRect.Left(), uvRect.Right(), uvRect.Right()};
	const f32 vs[4] = {uvRect.Top(), uvRect.Bottom(), uvRect.Bottom(), uvRect.Top()};

	Vertex verts[4];
	for (i32 i = 0; i < 4; ++i) {
		const glm::vec4 world = transform * corners[i];
		verts[i] = Vertex{world.x, world.y, z, us[i], vs[i], color.r, color.g, color.b, color.a,
			texIndex, entityToken};
	}
	submitVertices(verts);
}

Rect Renderer2D::CurrentClip() const {
	return _clips.empty() ? Rect(-1e20f, -1e20f, 2e20f, 2e20f) : _clips.back();
}

void Renderer2D::PushClip(const Rect &rect) {
	const Rect parent = CurrentClip();
	const f32 left = std::max(parent.x, rect.x), bottom = std::max(parent.y, rect.y);
	const f32 right = std::min(parent.Right(), rect.Right());
	const f32 top = std::min(parent.Top(), rect.Top());
	_clips.emplace_back(left, bottom, std::max(0.f, right - left), std::max(0.f, top - bottom));
}

void Renderer2D::PopClip() {
	if (!_clips.empty()) {
		_clips.pop_back();
	}
}

void Renderer2D::submitVertices(Vertex (&vertices)[4]) {
	const Rect clip = CurrentClip();
	f32 left = vertices[0].x, right = left, bottom = vertices[0].y, top = bottom;
	for (const auto &v : vertices) {
		left = std::min(left, v.x);
		right = std::max(right, v.x);
		bottom = std::min(bottom, v.y);
		top = std::max(top, v.y);
	}
	if (clip.w <= 0 || clip.h <= 0 || right <= clip.x || left >= clip.Right() || top <= clip.y ||
		bottom >= clip.Top()) {
		return;
	}
	for (auto &v : vertices) {
		v.clipLeft = clip.x;
		v.clipBottom = clip.y;
		v.clipRight = clip.Right();
		v.clipTop = clip.Top();
		_vertices.push_back(v);
	}
}

void Renderer2D::flush() {
	if (!_device || _vertices.empty()) {
		return;
	}

	const u32 vertexBytes = static_cast<u32>(_vertices.size() * sizeof(Vertex));
	_device->UpdateBuffer(_vertexBuffer, _vertices.data(), vertexBytes);

	_device->BindPipeline(
		_blendMode == ParticleBlendMode::Additive ? _additivePipeline : _pipeline);
	_device->SetUniformFloat("uAlphaCutoff", _alphaCutoff);
	_device->SetUniformMat4("uProj", _projection);
	_device->SetUniformMat4("uView", _view);

	i32 samplers[MaxTextures];
	for (u32 i = 0; i < MaxTextures; ++i) {
		samplers[i] = static_cast<i32>(i);
	}
	_device->SetUniformIntArray("uTextures", samplers, MaxTextures);

	// Bind a valid texture to every sampler unit the shader declares.
	// Leaving units empty triggers driver warnings like
	// "GLD_TEXTURE_INDEX_2D is unloadable" on macOS.
	for (u32 i = 0; i < MaxTextures; ++i) {
		_device->BindTexture(_whiteTexture, i);
	}
	for (const auto &[texture, slot] : _batchTextures) {
		_device->BindTexture(texture, slot);
	}

	const u32 indexCount = static_cast<u32>(_vertices.size() / 4 * 6);
	_device->DrawIndexed(_vertexBuffer, _indexBuffer, indexCount);
	++_stats.drawCalls;
	_stats.quads += static_cast<u32>(_vertices.size() / 4);
	_stats.textureBinds += static_cast<u32>(_batchTextures.size());

	_vertices.clear();
	_batchTextures.clear();
}

} // namespace sowa
