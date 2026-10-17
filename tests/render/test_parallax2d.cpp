#include <doctest/doctest.h>

#include <cmath>
#include <cstring>
#include <limits>

#include "core/filesystem/file_system.hpp"
#include "ecs/component_serializer.hpp"
#include "ecs/components/camera2d.hpp"
#include "ecs/components/parallax2d.hpp"
#include "ecs/world.hpp"
#include "math/matrix.hpp"
#include "render/renderer2d.hpp"
#include "render/sprite_renderer2d.hpp"
#include "render/stub_graphics_device.hpp"
#include "resource/image_texture.hpp"

using namespace sowa;

namespace {
struct Fixture {
	ResourceRegistry resources;
	World world;
	SpriteRenderer2D renderer;
	ResourceID texture;

	Fixture() {
		resources.RegisterType<ImageTexture>("ImageTexture");
		renderer.Init(world, resources);
		auto image = resources.Create<ImageTexture>();
		image->Adopt(resources.Context(), TextureHandle{1, 1}, 32, 32);
		texture = image->Id();
	}

	Entity SpriteAt(Vector2 position = {}) {
		auto entity = world.CreateEntity();
		entity.Add(Transform2D{.position = position});
		entity.Add(Sprite{.texture = texture, .centered = false});
		return entity;
	}

	void Collect(const glm::mat4 &vp = CalculateBlankViewProjection({96, 64}),
		Vector2 camera = {}) {
		world.Progress(0.f);
		renderer.Collect(vp, camera);
	}
};

bool Covers(const std::vector<SpriteRenderer2D::DrawItem> &items, glm::vec4 point) {
	for (const auto &item : items) {
		if (item.clip && !item.clip->Contains({point.x / point.w, point.y / point.w})) {
			continue;
		}
		const auto local = glm::inverse(glm::dmat4(item.transform)) * glm::dvec4(point);
		if (local.x / local.w >= item.localRect.x && local.x / local.w <= item.localRect.Right() &&
			local.y / local.w >= item.localRect.y && local.y / local.w <= item.localRect.Top()) {
			return true;
		}
	}
	return false;
}
} // namespace

TEST_CASE("Parallax2D camera speeds are independent and never mutate transforms") {
	Fixture f;
	auto entity = f.SpriteAt({10, 20});
	entity.Add(Parallax2D{.xSpeed = 0.25f, .ySpeed = 1.5f, .offset = {3, -4}});
	f.Collect(CalculateBlankViewProjection({96, 64}), {100, -60});
	REQUIRE(f.renderer.DrawList().size() == 1);
	const auto &matrix = f.renderer.DrawList()[0].transform;
	CHECK(matrix[3].x == doctest::Approx(88));
	CHECK(matrix[3].y == doctest::Approx(46));
	CHECK(entity.Get<Transform2D>()->position == Vector2(10, 20));
	CHECK(entity.Get<Transform2D>()->WorldPosition() == Vector2(10, 20));
	f.Collect(CalculateBlankViewProjection({96, 64}), {100, -60});
	CHECK(f.renderer.DrawList()[0].transform[3].x == doctest::Approx(88));
	entity.Get<Parallax2D>()->xSpeed = 1;
	entity.Get<Parallax2D>()->ySpeed = 0;
	f.Collect(CalculateBlankViewProjection({96, 64}), {100, -60});
	CHECK(f.renderer.DrawList()[0].transform[3].x == doctest::Approx(13));
	CHECK(f.renderer.DrawList()[0].transform[3].y == doctest::Approx(-44));
}

TEST_CASE("Parallax2D repeats each axis independently using texture size") {
	Fixture f;
	auto entity = f.SpriteAt();
	entity.Add(Parallax2D{});
	SUBCASE("X only") {
		entity.Get<Parallax2D>()->tileX = true;
		f.Collect();
		REQUIRE(f.renderer.DrawList().size() == 3);
		for (const auto &item : f.renderer.DrawList()) {
			CHECK(item.transform[3].y == 0);
		}
	}
	SUBCASE("Y only") {
		entity.Get<Parallax2D>()->tileY = true;
		f.Collect();
		REQUIRE(f.renderer.DrawList().size() == 2);
		for (const auto &item : f.renderer.DrawList()) {
			CHECK(item.transform[3].x == 0);
		}
	}
	SUBCASE("Both") {
		entity.Get<Parallax2D>()->tileX = true;
		entity.Get<Parallax2D>()->tileY = true;
		f.Collect();
		REQUIRE(f.renderer.DrawList().size() == 6);
	}
	SUBCASE("Neither") {
		f.Collect();
		CHECK(f.renderer.DrawList().size() == 1);
	}
	for (const auto &item : f.renderer.DrawList()) {
		CHECK(item.entity == entity);
		CHECK(item.texture.id == 1);
		CHECK(item.uvRect.w == 1);
	}
}

TEST_CASE("Parallax2D fills a rotated zoomed camera at large negative positions") {
	Fixture f;
	auto entity = f.SpriteAt({-3, 7});
	entity.Get<Transform2D>()->scale = {2, 0.5f};
	entity.Get<Transform2D>()->rotation = 25;
	entity.Get<Sprite>()->centered = true;
	entity.Get<Sprite>()->offset = {5, -9};
	entity.Add(Parallax2D{.tileX = true, .tileY = true});
	auto camera = f.world.CreateEntity();
	camera.Add(Transform2D{.position = {-12000, -43000}, .rotation = -15, .scale = {2, 3}});
	camera.Add(Camera2D{.offset = {4, 8}, .rotatable = true, .anchor = {0.2f, 0.8f}});
	const auto vp = CalculateViewProjection(camera, {96, 64});
	f.Collect(vp, CalculateCameraOrigin(camera, vp));
	REQUIRE(f.renderer.DrawList().size() > 6);
	const auto inverse = glm::inverse(vp);
	for (float x = -0.99f; x < 1; x += 0.11f) {
		for (float y = -0.99f; y < 1; y += 0.13f) {
			INFO("sample: ", x, ", ", y);
			CHECK(Covers(f.renderer.DrawList(), inverse * glm::vec4(x, y, 0, 1)));
		}
	}
}

TEST_CASE("Parallax2D uses the rendered camera origin with offset and inherited transforms") {
	Fixture f;
	auto entity = f.SpriteAt({10, 20});
	entity.Add(Parallax2D{.xSpeed = 0.25f, .ySpeed = 1.5f, .offset = {3, -4}});
	auto parent = f.world.CreateEntity();
	parent.Add(Transform2D{.position = {100, -50}, .rotation = 90, .scale = {2, 2}});
	auto camera = f.world.CreateEntity();
	camera.SetParent(parent);
	camera.Add(Transform2D{.position = {10, 20}, .scale = {2, 3}});
	camera.Add(Camera2D{.offset = {4, 8}, .rotatable = true, .anchor = {0.2f, 0.8f}});

	// Parent places the camera at (60, -30), with world scale (4, 6) and
	// rotation 90 degrees. Its offset rotates to (-8, 4), without scaling.
	for (Vector2 anchor : {Vector2(0.2f, 0.8f), Vector2(0.5f, 0.5f), Vector2(1, 0)}) {
		camera.Get<Camera2D>()->anchor = anchor;
		const auto vp = CalculateViewProjection(camera, {96, 64});
		const auto origin = CalculateCameraOrigin(camera, vp);
		CHECK(origin.x == doctest::Approx(52));
		CHECK(origin.y == doctest::Approx(-26));
		f.Collect(vp, origin);
		REQUIRE(f.renderer.DrawList().size() == 1);
		const auto &matrix = f.renderer.DrawList()[0].transform;
		CHECK(matrix[3].x == doctest::Approx(52));
		CHECK(matrix[3].y == doctest::Approx(29));
	}

	camera.Get<Camera2D>()->rotatable = false;
	const auto vp = CalculateViewProjection(camera, {96, 64});
	const auto origin = CalculateCameraOrigin(camera, vp);
	CHECK(origin.x == doctest::Approx(64));
	CHECK(origin.y == doctest::Approx(-22));
	f.Collect(vp, origin);
	CHECK(f.renderer.DrawList()[0].transform[3].x == doctest::Approx(61));
	CHECK(f.renderer.DrawList()[0].transform[3].y == doctest::Approx(27));
	CHECK(entity.Get<Transform2D>()->position == Vector2(10, 20));

	CHECK(CalculateCameraOrigin({}, vp) == Vector2());
}

TEST_CASE("Parallax2D limits clip partial tiles at fixed world positions") {
	Fixture f;
	auto entity = f.SpriteAt();
	entity.Add(Parallax2D{
		.tileX = true,
		.tileY = true,
		.xSpeed = 0.25f,
		.ySpeed = 0.75f,
		.limitX = true,
		.limitY = true,
		.limitMin = {10, 9},
		.limitMax = {77, 51},
	});

	for (Vector2 camera : {Vector2(0, 0), Vector2(137, -94)}) {
		f.Collect(CalculateBlankViewProjection({96, 64}), camera);
		const auto &items = f.renderer.DrawList();
		REQUIRE_FALSE(items.empty());
		CHECK(Covers(items, {10.1f, 9.1f, 0, 1}));
		CHECK(Covers(items, {76.9f, 50.9f, 0, 1}));
		CHECK_FALSE(Covers(items, {9.9f, 30, 0, 1}));
		CHECK_FALSE(Covers(items, {77.1f, 30, 0, 1}));
		CHECK_FALSE(Covers(items, {40, 8.9f, 0, 1}));
		CHECK_FALSE(Covers(items, {40, 51.1f, 0, 1}));
		for (const auto &item : items) {
			REQUIRE(item.clip.has_value());
			CHECK(item.clip->x == 10);
			CHECK(item.clip->y == 9);
			CHECK(item.clip->Right() == 77);
			CHECK(item.clip->Top() == 51);
		}
	}

	SUBCASE("A disabled axis is not limited") {
		entity.Get<Parallax2D>()->limitY = false;
		f.Collect();
		CHECK(Covers(f.renderer.DrawList(), {40, 1, 0, 1}));
		CHECK(Covers(f.renderer.DrawList(), {40, 63, 0, 1}));
	}
	SUBCASE("A viewport beyond the limits has no tiles") {
		f.Collect(Matrix::CalculateOrtho(100, 200, 100, 200), {150, 150});
		CHECK(f.renderer.DrawList().empty());
	}
	SUBCASE("Empty or reversed limits have no tiles") {
		entity.Get<Parallax2D>()->limitMax.x = 10;
		f.Collect();
		CHECK(f.renderer.DrawList().empty());
		entity.Get<Parallax2D>()->limitMax.x = -10;
		f.Collect();
		CHECK(f.renderer.DrawList().empty());
	}
}

TEST_CASE("Parallax2D repeats a parent layer as a unit and nearest layer wins") {
	Fixture f;
	auto parent = f.world.CreateEntity();
	parent.Add(Transform2D{.position = {10, 20}, .scale = {2, 2}});
	parent.Add(Parallax2D{.tileX = true, .xSpeed = 0, .ySpeed = 1});
	auto a = f.SpriteAt();
	auto b = f.SpriteAt({32, 0});
	a.SetParent(parent);
	b.SetParent(parent);
	f.Collect(CalculateBlankViewProjection({256, 128}), {100, 0});
	CHECK(Covers(f.renderer.DrawList(), {1, 21, 0, 1}));
	CHECK(Covers(f.renderer.DrawList(), {255, 21, 0, 1}));
	for (const auto &item : f.renderer.DrawList()) {
		CHECK(item.transform[3].y == 20);
		CHECK(item.transform[0].x == 2);
	}
	b.Add(Parallax2D{.xSpeed = 1, .ySpeed = 1});
	f.Collect(CalculateBlankViewProjection({256, 128}), {100, 0});
	int bCount = 0;
	for (const auto &item : f.renderer.DrawList()) {
		if (item.entity == b) {
			++bCount;
			CHECK(item.transform[3].x == 74);
		}
	}
	CHECK(bCount == 1);
	parent.SetVisible(false);
	f.Collect();
	CHECK(f.renderer.DrawList().empty());
}

TEST_CASE("Parallax2D uses the sprite size without a separate repeat parameter") {
	Fixture f;
	auto entity = f.SpriteAt();
	entity.Add(Parallax2D{.tileX = true, .tileY = true});
	entity.Get<Sprite>()->size = {48, 32};
	f.Collect();
	CHECK(f.renderer.DrawList().size() == 4);
	entity.Get<Sprite>()->size = {96, 64};
	f.Collect();
	CHECK(f.renderer.DrawList().size() == 1);
	CHECK(f.world.FindComponent("Parallax2D").FindField("repeatSize") == nullptr);

	entity.Get<Sprite>()->size = {1e-20f, 1e-20f};
	f.Collect();
	CHECK(f.renderer.DrawList().empty());
	entity.Get<Transform2D>()->scale = {0, 0};
	f.Collect();
	CHECK(f.renderer.DrawList().empty());
}

TEST_CASE("Parallax2D offset is script-editable and settings survive serialization") {
	Fixture f;
	auto entity = f.SpriteAt();
	entity.Add(Parallax2D{.tileX = true,
		.tileY = true,
		.xSpeed = -0.5f,
		.ySpeed = 2,
		.offset = {10, -20},
		.limitX = true,
		.limitY = true,
		.limitMin = {-100, -200},
		.limitMax = {100, 200}});
	const auto type = f.world.FindComponent("Parallax2D");
	REQUIRE(type.IsValid());
	REQUIRE(type.FindField("offset") != nullptr);
	Archive saving = Archive::Save();
	REQUIRE(SerializeComponent(saving, entity, type));
	auto copy = f.world.CreateEntity();
	Archive loading = Archive::Load(saving.ToString());
	REQUIRE(SerializeComponent(loading, copy, type));
	CHECK_FALSE(loading.HasErrors());
	const auto &p = *copy.Get<Parallax2D>();
	CHECK(p.tileX);
	CHECK(p.tileY);
	CHECK(p.xSpeed == -0.5f);
	CHECK(p.ySpeed == 2);
	CHECK(p.offset == Vector2(10, -20));
	CHECK(p.limitX);
	CHECK(p.limitY);
	CHECK(p.limitMin == Vector2(-100, -200));
	CHECK(p.limitMax == Vector2(100, 200));
	f.world.Progress(1);
	CHECK(copy.Get<Parallax2D>()->offset == Vector2(10, -20));
}

TEST_CASE("Parallax2D sends clipping to the shader without clipping ordinary sprites") {
	struct Device : StubGraphicsDevice {
		std::vector<u8> vertices;
		u32 bufferId = 0;

		PipelineHandle CreatePipeline(const PipelineDesc &) override {
			return {1, 1};
		}

		BufferHandle CreateBuffer(BufferType, BufferUsage, u32, const void *) override {
			return {++bufferId, 1};
		}

		void UpdateBuffer(BufferHandle, const void *data, u32 size, u32) override {
			const auto *bytes = static_cast<const u8 *>(data);
			vertices.insert(vertices.end(), bytes, bytes + size);
		}
	};

	Device device;
	FileSystem fs;
	fs.MountFolder("res", SOWA_EXAMPLE_DIR);
	Renderer2D gpu;
	REQUIRE(gpu.Init(device, fs));
	gpu.SetProjection(96, 64);

	Fixture f;
	auto bounded = f.SpriteAt();
	bounded.Add(Parallax2D{
		.tileX = true,
		.tileY = true,
		.limitX = true,
		.limitY = true,
		.limitMin = {7, 9},
		.limitMax = {27, 25},
	});
	bounded.Add(RenderOrder{.zIndex = -1});
	f.SpriteAt({80, 40});
	f.world.Progress(0);

	EntityPickMap picks;
	f.renderer.Draw(gpu, picks);

	// The renderer layout is 10 floats, a pick token, and 4 clip floats.
	constexpr size_t vertexBytes = 60;
	REQUIRE(device.vertices.size() == 8 * vertexBytes);
	float vertex[15];
	std::memcpy(vertex, device.vertices.data(), vertexBytes);
	CHECK(vertex[11] == 7);
	CHECK(vertex[12] == 9);
	CHECK(vertex[13] == 27);
	CHECK(vertex[14] == 25);
	std::memcpy(vertex, device.vertices.data() + 4 * vertexBytes, vertexBytes);
	CHECK(vertex[11] < -1e10f);
	CHECK(vertex[13] > 1e10f);
}
