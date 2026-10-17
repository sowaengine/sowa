#include <doctest/doctest.h>
#include <vector>

#include "core/filesystem/file_system.hpp"
#include "render/renderer2d.hpp"
#include "render/stub_graphics_device.hpp"

using namespace sowa;

namespace {
struct ParticleDevice : StubGraphicsDevice {
	std::vector<PipelineDesc> pipelines;
	std::vector<PipelineHandle> draws;
	std::vector<f32> cutoffs;
	PipelineHandle current;
	i32 buffers = 0, destroyedPipelines = 0;

	PipelineHandle CreatePipeline(const PipelineDesc &desc) override {
		pipelines.push_back(desc);
		return {static_cast<u32>(pipelines.size()), 1};
	}

	void DestroyPipeline(PipelineHandle) override { ++destroyedPipelines; }

	BufferHandle CreateBuffer(BufferType, BufferUsage, u32, const void *) override {
		return {static_cast<u32>(++buffers), 1};
	}

	void BindPipeline(PipelineHandle handle) override { current = handle; }

	void DrawIndexed(BufferHandle, BufferHandle, u32) override { draws.push_back(current); }

	void SetUniformFloat(const char *name, f32 value) override {
		if (std::string_view(name) == "uAlphaCutoff") {
			cutoffs.push_back(value);
		}
	}

	TextureHandle CreateTexture(i32 width, i32 height, const void *pixels,
								TextureFilter filter) override {
		if (width == 64 && height == 64) {
			const auto *rgba = static_cast<const u8 *>(pixels);
			CHECK(rgba[3] == 0); // transparent corners, opaque centers
			CHECK(rgba[(32 * 64 + 32) * 4 + 3] >= 250);
		}
		return StubGraphicsDevice::CreateTexture(width, height, pixels, filter);
	}
};
} // namespace

TEST_CASE("Particles: renderer flushes blend/cutoff changes and releases shared masks") {
	ParticleDevice device;
	FileSystem fs;
	fs.MountFolder("res", SOWA_EXAMPLE_DIR);
	Renderer2D renderer;
	REQUIRE(renderer.Init(device, fs));
	REQUIRE(device.pipelines.size() == 2);
	CHECK_FALSE(device.pipelines[0].additiveBlend);
	CHECK(device.pipelines[1].additiveBlend);
	CHECK(device.textureCreateCount == 3);
	CHECK(renderer.ParticleTexture(ParticleDrawShape::SoftCircle).IsValid());
	CHECK(renderer.ParticleTexture(ParticleDrawShape::Circle).IsValid());
	CHECK(renderer.ParticleTexture(ParticleDrawShape::SoftCircle) !=
		  renderer.ParticleTexture(ParticleDrawShape::Circle));
	renderer.Begin();
	renderer.DrawQuad(0, 0, 1, 1, Color{});
	renderer.SetBlendMode(ParticleBlendMode::Additive);
	renderer.SetAlphaCutoff(0.001f);
	renderer.DrawQuad(0, 0, 1, 1, renderer.ParticleTexture(ParticleDrawShape::SoftCircle));
	renderer.SetBlendMode(ParticleBlendMode::Alpha);
	renderer.DrawQuad(0, 0, 1, 1, Color{});
	renderer.End();
	REQUIRE(device.draws.size() == 3);
	CHECK(device.draws[0].id == 1);
	CHECK(device.draws[1].id == 2);
	CHECK(device.draws[2].id == 1);
	CHECK(device.cutoffs == std::vector<f32>{0.1f, 0.001f, 0.001f});
	renderer.Begin();
	renderer.DrawQuad(0, 0, 1, 1, Color{});
	renderer.End();
	CHECK(device.cutoffs.back() == 0.1f);
	renderer.Shutdown();
	CHECK(device.textureDestroyCount == 3);
	CHECK(device.destroyedPipelines == 2);
}

TEST_CASE("Particles: large additive batches retain their blend mode across capacity flushes") {
	ParticleDevice device;
	FileSystem fs;
	fs.MountFolder("res", SOWA_EXAMPLE_DIR);
	Renderer2D renderer;
	REQUIRE(renderer.Init(device, fs));
	renderer.Begin();
	renderer.SetBlendMode(ParticleBlendMode::Additive);
	for (int i = 0; i < 2100; ++i) {
		renderer.DrawQuad(0, 0, 1, 1, Color{});
	}
	renderer.End();
	REQUIRE(device.draws.size() == 3);
	for (auto pipeline : device.draws) {
		CHECK(pipeline.id == 2);
	}
}
