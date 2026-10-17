#include "render/fullscreen_present.hpp"

#include "assets/embedded.hpp"
#include "core/log.hpp"
#include "render/shader.hpp"

namespace sowa {
namespace {

struct FullscreenVertex {
	f32 x = 0.f;
	f32 y = 0.f;
	f32 u = 0.f;
	f32 v = 0.f;
};

} // namespace

FullscreenPresent::~FullscreenPresent() {
	Shutdown();
}

bool FullscreenPresent::Init(GraphicsDevice &device, FileSystem &fs, std::string_view shaderPath) {
	Shutdown();
	_device = &device;

	if (shaderPath.empty()) {
		const auto source = assets::FullscreenShader();
		_shader = CreateShaderFromSource(device,
			std::string_view(reinterpret_cast<const char *>(source.data()), source.size()));
	} else {
		_shader = CreateShaderFromFile(device, fs, shaderPath);
	}
	if (!_shader.IsValid()) {
		Log::Error("FullscreenPresent: failed to load shader {}", shaderPath);
		Shutdown();
		return false;
	}

	PipelineDesc pipelineDesc;
	pipelineDesc.shader = _shader;
	pipelineDesc.alphaBlend = false;
	pipelineDesc.layout.attributes = {
		{0, VertexAttrType::Vec2},
		{1, VertexAttrType::Vec2},
	};
	_pipeline = device.CreatePipeline(pipelineDesc);
	if (!_pipeline.IsValid()) {
		Log::Error("FullscreenPresent: failed to create pipeline");
		Shutdown();
		return false;
	}

	const FullscreenVertex verts[4] = {
		{-1.f, -1.f, 0.f, 0.f},
		{1.f, -1.f, 1.f, 0.f},
		{1.f, 1.f, 1.f, 1.f},
		{-1.f, 1.f, 0.f, 1.f},
	};
	_vertexBuffer = device.CreateBuffer(BufferType::Vertex, BufferUsage::Static,
		static_cast<u32>(sizeof(verts)), verts);
	if (!_vertexBuffer.IsValid()) {
		Log::Error("FullscreenPresent: failed to create vertex buffer");
		Shutdown();
		return false;
	}

	const u32 indices[6] = {0, 1, 2, 0, 2, 3};
	_indexBuffer =
		device.CreateBuffer(BufferType::Index, BufferUsage::Static, sizeof(indices), indices);
	if (!_indexBuffer.IsValid()) {
		Log::Error("FullscreenPresent: failed to create index buffer");
		Shutdown();
		return false;
	}

	return true;
}

void FullscreenPresent::Shutdown() {
	if (!_device) {
		return;
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
	_device = nullptr;
}

void FullscreenPresent::Draw(TextureHandle texture) {
	if (!_device || !texture.IsValid()) {
		return;
	}

	_device->BindPipeline(_pipeline);
	_device->SetUniformTexture("uTexture", texture, 0);
	_device->DrawIndexed(_vertexBuffer, _indexBuffer, 6);
}

} // namespace sowa
