#include <vector>

#include <doctest/doctest.h>

#include "ecs/components/render_order.hpp"
#include "ecs/components/sprite.hpp"
#include "ecs/components/transform2d.hpp"
#include "ecs/entity.hpp"
#include "ecs/world.hpp"
#include "render/sprite_renderer2d.hpp"
#include "resource/image_texture.hpp"
#include "resource/resource_registry.hpp"

using namespace sowa;

namespace {
void checkRect(Rect actual, Rect expected) {
	CHECK(actual.x == doctest::Approx(expected.x));
	CHECK(actual.y == doctest::Approx(expected.y));
	CHECK(actual.w == doctest::Approx(expected.w));
	CHECK(actual.h == doctest::Approx(expected.h));
}
} // namespace

namespace {

// A registry with no graphics device: Adopt lets a texture report a handle and
// a size without one, which is the whole reason it isn't a test-only hook.
// Handles are opaque ids, so a fabricated one is enough to sort and batch.
struct Textures {
	ResourceRegistry registry;

	Textures() { registry.RegisterType<ImageTexture>("ImageTexture"); }

	ResourceID Make(u32 handleId, i32 size = 32) {
		Ref<ImageTexture> texture = registry.Create<ImageTexture>();
		texture->Adopt(registry.Context(), TextureHandle{handleId, 1}, size, size);
		return texture->Id();
	}
};

Sprite MakeSprite(ResourceID texture, f32 size = 10.f) {
	Sprite sprite;
	sprite.texture = texture;
	sprite.size = Vector2(size, size);
	return sprite;
}

Entity SpawnSprite(World &world, ResourceID texture, Vector2 position, i32 zIndex) {
	Entity entity = world.CreateEntity();
	Transform2D transform;
	transform.position = position;
	entity.Add(transform);
	entity.Add(MakeSprite(texture));
	entity.Add(RenderOrder{zIndex});
	return entity;
}

} // namespace

TEST_CASE("Sprite::LocalRect centres on the origin by default") {
	Sprite sprite = MakeSprite(ResourceID{1}, 20.f);

	const Rect rect = sprite.LocalRect(sprite.size);
	CHECK(rect.x == doctest::Approx(-10.f));
	CHECK(rect.y == doctest::Approx(-10.f));
	CHECK(rect.w == doctest::Approx(20.f));
	CHECK(rect.h == doctest::Approx(20.f));
}

TEST_CASE("Sprite::LocalRect grows from the origin when not centred") {
	Sprite sprite = MakeSprite(ResourceID{1}, 20.f);
	sprite.centered = false;

	const Rect rect = sprite.LocalRect(sprite.size);
	CHECK(rect.x == doctest::Approx(0.f));
	CHECK(rect.y == doctest::Approx(0.f));
	CHECK(rect.w == doctest::Approx(20.f));
}

TEST_CASE("Sprite::LocalRect applies the offset in both modes") {
	Sprite centred = MakeSprite(ResourceID{1}, 20.f);
	centred.offset = Vector2(5.f, -3.f);
	CHECK(centred.LocalRect(centred.size).x == doctest::Approx(-5.f));
	CHECK(centred.LocalRect(centred.size).y == doctest::Approx(-13.f));

	Sprite cornered = MakeSprite(ResourceID{1}, 20.f);
	cornered.centered = false;
	cornered.offset = Vector2(5.f, -3.f);
	CHECK(cornered.LocalRect(cornered.size).x == doctest::Approx(5.f));
	CHECK(cornered.LocalRect(cornered.size).y == doctest::Approx(-3.f));
}

TEST_CASE("An unset size falls back to the texture's own dimensions") {
	Sprite sprite;
	sprite.texture = ResourceID{1};

	CHECK(sprite.ResolvedSize(Vector2(64.f, 32.f)).x == doctest::Approx(64.f));
	CHECK(sprite.ResolvedSize(Vector2(64.f, 32.f)).y == doctest::Approx(32.f));

	// An explicit size wins, and does so as a whole rather than per axis.
	sprite.size = Vector2(10.f, 20.f);
	CHECK(sprite.ResolvedSize(Vector2(64.f, 32.f)).x == doctest::Approx(10.f));
	CHECK(sprite.ResolvedSize(Vector2(64.f, 32.f)).y == doctest::Approx(20.f));
}

TEST_CASE("A texture-sized sprite draws at the texture's dimensions") {
	Textures textures;
	World world;
	SpriteRenderer2D renderer;
	renderer.Init(world, textures.registry);

	Entity entity = world.CreateEntity();
	entity.Add(Transform2D{});
	Sprite sprite;
	sprite.texture = textures.Make(1, 48); // leaves size at zero
	entity.Add(sprite);

	world.Progress(0.f);
	renderer.Collect();

	REQUIRE(renderer.DrawList().size() == 1);
	CHECK(renderer.DrawList()[0].localRect.w == doctest::Approx(48.f));
	CHECK(renderer.DrawList()[0].localRect.h == doctest::Approx(48.f));
}

TEST_CASE("Sprite is registered with its serializable fields, texture included") {
	Textures textures;
	World world;
	SpriteRenderer2D renderer;
	renderer.Init(world, textures.registry);

	const ComponentType type = world.FindComponent("Sprite");
	REQUIRE(type.IsValid());

	CHECK(type.FindField("modulate") != nullptr);
	CHECK(type.FindField("size") != nullptr);
	CHECK(type.FindField("offset") != nullptr);
	CHECK(type.FindField("centered") != nullptr);

	// ResourceID fields survive serialization; graphics handles do not.
	const FieldInfo *texture = type.FindField("texture");
	REQUIRE(texture != nullptr);
	CHECK(texture->type == FieldType::ResourceRef);
}

TEST_CASE("Collect gathers every drawable sprite") {
	Textures textures;
	World world;
	SpriteRenderer2D renderer;
	renderer.Init(world, textures.registry);

	SpawnSprite(world, textures.Make(1), Vector2(0.f, 0.f), 0);
	SpawnSprite(world, textures.Make(1), Vector2(10.f, 0.f), 0);
	world.Progress(0.f);

	renderer.Collect();
	CHECK(renderer.DrawList().size() == 2);
}

TEST_CASE("Sprites whose texture is missing, unloaded or empty are skipped") {
	Textures textures;
	World world;
	SpriteRenderer2D renderer;
	renderer.Init(world, textures.registry);

	SpawnSprite(world, textures.Make(1), Vector2(0.f, 0.f), 0);

	// No texture at all.
	Entity blank = world.CreateEntity();
	blank.Add(Transform2D{});
	blank.Add(MakeSprite(ResourceID{}));

	// An id that resolves to nothing - a scene referencing a deleted resource.
	Entity dangling = world.CreateEntity();
	dangling.Add(Transform2D{});
	dangling.Add(MakeSprite(ResourceID{12345}));

	// A real resource that never loaded, e.g. a missing file on disk.
	Entity unloaded = world.CreateEntity();
	unloaded.Add(Transform2D{});
	unloaded.Add(MakeSprite(textures.registry.Create<ImageTexture>()->Id()));

	// Loaded, but explicitly sized to nothing.
	Entity sizeless = world.CreateEntity();
	sizeless.Add(Transform2D{});
	Sprite empty = MakeSprite(textures.Make(2));
	empty.size = Vector2(0.f, 4.f);
	sizeless.Add(empty);

	world.Progress(0.f);
	renderer.Collect();

	// Degenerate quads never reach the batch at all.
	CHECK(renderer.DrawList().size() == 1);
}

TEST_CASE("The draw list is ordered by effective z-index") {
	Textures textures;
	World world;
	SpriteRenderer2D renderer;
	renderer.Init(world, textures.registry);

	Entity top = SpawnSprite(world, textures.Make(1), Vector2(0.f, 0.f), 10);
	Entity middle = SpawnSprite(world, textures.Make(1), Vector2(0.f, 0.f), 5);
	Entity bottom = SpawnSprite(world, textures.Make(1), Vector2(0.f, 0.f), -3);

	world.Progress(0.f);
	renderer.Collect();

	const auto &list = renderer.DrawList();
	REQUIRE(list.size() == 3);
	CHECK(list[0].key.zIndex == -3);
	CHECK(list[0].entity == bottom);
	CHECK(list[1].key.zIndex == 5);
	CHECK(list[1].entity == middle);
	CHECK(list[2].key.zIndex == 10);
	CHECK(list[2].entity == top);
}

TEST_CASE("Layer beats z-index in the draw list") {
	Textures textures;
	World world;
	SpriteRenderer2D renderer;
	renderer.Init(world, textures.registry);

	Entity ground = SpawnSprite(world, textures.Make(1), Vector2(0.f, 0.f), 9999);
	Entity hud = world.CreateEntity();
	hud.Add(Transform2D{});
	hud.Add(MakeSprite(textures.Make(1)));
	hud.Add(RenderOrder{-9999, ZIndexMode::Absolute, 1, RenderLayerMode::Explicit});

	world.Progress(0.f);
	renderer.Collect();

	const auto &list = renderer.DrawList();
	REQUIRE(list.size() == 2);
	// Submitted last means drawn on top, painter's order.
	CHECK(list[0].key.zIndex == 9999);
	CHECK(list[1].key.layer == 1);
	(void)ground;
}

TEST_CASE("A child inherits its parent's z-index into the draw order") {
	Textures textures;
	World world;
	SpriteRenderer2D renderer;
	renderer.Init(world, textures.registry);

	Entity parent = SpawnSprite(world, textures.Make(1), Vector2(0.f, 0.f), 100);
	Entity shadow = world.CreateEntity();
	shadow.Add(Transform2D{});
	shadow.Add(MakeSprite(textures.Make(1)));
	shadow.Add(RenderOrder{-1}); // relative: behind its parent, not everything
	REQUIRE(shadow.SetParent(parent));

	Entity distant = SpawnSprite(world, textures.Make(1), Vector2(0.f, 0.f), 0);

	world.Progress(0.f);
	renderer.Collect();

	const auto &list = renderer.DrawList();
	REQUIRE(list.size() == 3);
	CHECK(list[0].key.zIndex == 0);	  // distant
	CHECK(list[1].key.zIndex == 99);  // shadow, behind its parent
	CHECK(list[2].key.zIndex == 100); // parent
	(void)distant;
}

TEST_CASE("Equal keys group by texture so the batch flushes less") {
	Textures textures;
	World world;
	SpriteRenderer2D renderer;
	renderer.Init(world, textures.registry);

	const ResourceID first = textures.Make(1);
	const ResourceID second = textures.Make(2);

	// Interleaved on purpose; all at the same z, where order is unspecified
	// and grouping is free.
	SpawnSprite(world, second, Vector2(0.f, 0.f), 0);
	SpawnSprite(world, first, Vector2(0.f, 0.f), 0);
	SpawnSprite(world, second, Vector2(0.f, 0.f), 0);
	SpawnSprite(world, first, Vector2(0.f, 0.f), 0);

	world.Progress(0.f);
	renderer.Collect();

	const auto &list = renderer.DrawList();
	REQUIRE(list.size() == 4);
	CHECK(list[0].textureId == 1);
	CHECK(list[1].textureId == 1);
	CHECK(list[2].textureId == 2);
	CHECK(list[3].textureId == 2);
}

TEST_CASE("Texture grouping never reorders across different keys") {
	Textures textures;
	World world;
	SpriteRenderer2D renderer;
	renderer.Init(world, textures.registry);

	// Texture 2 sits below texture 1 by z-index. Grouping must not lift it.
	SpawnSprite(world, textures.Make(2), Vector2(0.f, 0.f), 0);
	SpawnSprite(world, textures.Make(1), Vector2(0.f, 0.f), 1);
	SpawnSprite(world, textures.Make(2), Vector2(0.f, 0.f), 2);

	world.Progress(0.f);
	renderer.Collect();

	const auto &list = renderer.DrawList();
	REQUIRE(list.size() == 3);
	CHECK(list[0].key.zIndex == 0);
	CHECK(list[1].key.zIndex == 1);
	CHECK(list[2].key.zIndex == 2);
}

TEST_CASE("Hidden sprites and their subtrees never reach the draw list") {
	Textures textures;
	World world;
	SpriteRenderer2D renderer;
	renderer.Init(world, textures.registry);

	Entity panel = SpawnSprite(world, textures.Make(1), Vector2(0.f, 0.f), 0);
	Entity child = world.CreateEntity();
	child.Add(Transform2D{});
	child.Add(MakeSprite(textures.Make(1)));
	REQUIRE(child.SetParent(panel));
	Entity loose = SpawnSprite(world, textures.Make(1), Vector2(50.f, 0.f), 0);

	world.Progress(0.f);
	renderer.Collect();
	REQUIRE(renderer.DrawList().size() == 3);

	panel.SetVisible(false);
	world.Progress(0.f);
	renderer.Collect();

	// The child carries no visibility state of its own and still drops out.
	CHECK(renderer.DrawList().size() == 1);
	(void)loose;
}

TEST_CASE("The draw list carries each sprite's world matrix, not its local one") {
	Textures textures;
	World world;
	SpriteRenderer2D renderer;
	renderer.Init(world, textures.registry);

	Entity parent = world.CreateEntity();
	Transform2D parentTransform;
	parentTransform.position = Vector2(100.f, 50.f);
	parent.Add(parentTransform);

	Entity child = world.CreateEntity();
	Transform2D childTransform;
	childTransform.position = Vector2(10.f, 5.f);
	child.Add(childTransform);
	child.Add(MakeSprite(textures.Make(1)));
	REQUIRE(child.SetParent(parent));

	world.Progress(0.f);
	renderer.Collect();

	REQUIRE(renderer.DrawList().size() == 1);
	const glm::mat4 &matrix = renderer.DrawList()[0].transform;
	// Composed with the parent - drawing the local transform would put every
	// child near the origin.
	CHECK(matrix[3][0] == doctest::Approx(110.f));
	CHECK(matrix[3][1] == doctest::Approx(55.f));
}

TEST_CASE("A rotated entity's sprite corners rotate with it") {
	Textures textures;
	World world;
	SpriteRenderer2D renderer;
	renderer.Init(world, textures.registry);

	Entity entity = world.CreateEntity();
	Transform2D transform;
	transform.rotation = 90.f; // counter-clockwise, 0 degrees along +X
	entity.Add(transform);
	Sprite sprite = MakeSprite(textures.Make(1), 20.f);
	sprite.centered = false; // corner at the origin makes the mapping obvious
	entity.Add(sprite);

	world.Progress(0.f);
	renderer.Collect();
	REQUIRE(renderer.DrawList().size() == 1);

	// Validate world-matrix composition used to map sprite corners. Vertex upload needs a device.
	const SpriteRenderer2D::DrawItem &item = renderer.DrawList()[0];
	const Rect &rect = item.localRect;
	const glm::vec4 farCorner =
		item.transform * glm::vec4(rect.Right(), rect.Bottom(), 0.f, 1.f);

	// Local (20, 0) swings to world (0, 20) under +90 degrees.
	CHECK(farCorner.x == doctest::Approx(0.f).epsilon(0.001f));
	CHECK(farCorner.y == doctest::Approx(20.f).epsilon(0.001f));
}

TEST_CASE("A parent's scale reaches its child's sprite corners") {
	Textures textures;
	World world;
	SpriteRenderer2D renderer;
	renderer.Init(world, textures.registry);

	Entity parent = world.CreateEntity();
	Transform2D parentTransform;
	parentTransform.scale = Vector2(3.f, 3.f);
	parent.Add(parentTransform);

	Entity child = world.CreateEntity();
	child.Add(Transform2D{});
	Sprite sprite = MakeSprite(textures.Make(1), 10.f);
	sprite.centered = false;
	child.Add(sprite);
	REQUIRE(child.SetParent(parent));

	world.Progress(0.f);
	renderer.Collect();
	REQUIRE(renderer.DrawList().size() == 1);

	const SpriteRenderer2D::DrawItem &item = renderer.DrawList()[0];
	const glm::vec4 corner =
		item.transform * glm::vec4(item.localRect.Right(), item.localRect.Top(), 0.f, 1.f);

	// The sprite is 10 wide and inherits 3x, so the quad spans 30 on screen
	// even though nothing rescaled the sprite itself.
	CHECK(corner.x == doctest::Approx(30.f).epsilon(0.001f));
	CHECK(corner.y == doctest::Approx(30.f).epsilon(0.001f));
}

TEST_CASE("Collect is safe before Init and clears between frames") {
	SpriteRenderer2D uninitialised;
	uninitialised.Collect();
	CHECK(uninitialised.DrawList().empty());

	Textures textures;
	World world;
	SpriteRenderer2D renderer;
	renderer.Init(world, textures.registry);
	Entity entity = SpawnSprite(world, textures.Make(1), Vector2(0.f, 0.f), 0);
	world.Progress(0.f);

	renderer.Collect();
	renderer.Collect();
	// Not appended to on the second pass.
	CHECK(renderer.DrawList().size() == 1);

	entity.Destroy();
	world.Progress(0.f);
	renderer.Collect();
	CHECK(renderer.DrawList().empty());
}

TEST_CASE("NinePatchRect: collection preserves placement, parent transforms and render order") {
	Textures textures;
	World world;
	SpriteRenderer2D renderer;
	renderer.Init(world, textures.registry);
	auto parent = world.CreateEntity();
	Transform2D parentTransform;
	parentTransform.position = {100, 200};
	parentTransform.rotation = 90;
	parentTransform.scale = {2, 3};
	parent.Add(parentTransform);
	parent.Add(RenderOrder{7});
	auto entity = parent.CreateChild("Panel");
	Transform2D transform;
	transform.position = {10, 20};
	entity.Add(transform);
	NinePatchRect panel;
	panel.texture = textures.Make(55, 64);
	panel.paddingLeft = 4;
	panel.paddingTop = 6;
	panel.paddingRight = 8;
	panel.paddingBottom = 10;
	panel.size = {100, 80};
	panel.offset = {3, 5};
	panel.modulate = {.2f, .4f, .6f, .8f};
	entity.Add(panel);
	world.Progress(0);
	renderer.Collect();
	REQUIRE(renderer.DrawList().size() == 9);
	const auto &first = renderer.DrawList().front();
	CHECK(first.entity == entity);
	CHECK(first.key.zIndex == 7);
	CHECK(first.texture.id == 55);
	checkRect(first.localRect, Rect(-47, 39, 4, 6));
	CHECK(first.modulate == panel.modulate);
	CHECK(first.transform == entity.Get<Transform2D>()->worldMatrix);
	const glm::vec4 origin = first.transform * glm::vec4(0, 0, 0, 1);
	CHECK(origin.x == doctest::Approx(40));
	CHECK(origin.y == doctest::Approx(220));
	checkRect(renderer.DrawList().back().localRect, Rect(45, -35, 8, 10));

	entity.Get<NinePatchRect>()->centered = false;
	entity.Get<NinePatchRect>()->drawCenter = false;
	renderer.Collect();
	REQUIRE(renderer.DrawList().size() == 8);
	checkRect(renderer.DrawList().front().localRect, Rect(3, 79, 4, 6));
	parent.SetVisible(false);
	renderer.Collect();
	CHECK(renderer.DrawList().empty());
}

TEST_CASE("NinePatchRect: missing textures and empty destinations are skipped") {
	Textures textures;
	World world;
	SpriteRenderer2D renderer;
	renderer.Init(world, textures.registry);
	auto entity = world.CreateEntity();
	entity.Add<Transform2D>();
	entity.Add<NinePatchRect>();
	world.Progress(0);
	renderer.Collect();
	CHECK(renderer.DrawList().empty());
	entity.Get<NinePatchRect>()->texture = textures.Make(20);
	entity.Get<NinePatchRect>()->size = {-1, 40};
	renderer.Collect();
	CHECK(renderer.DrawList().empty());
}
