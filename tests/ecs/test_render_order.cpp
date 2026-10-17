#include <algorithm>
#include <string>
#include <utility>
#include <vector>

#include <doctest/doctest.h>

#include "ecs/components/render_order.hpp"
#include "ecs/entity.hpp"
#include "core/reflection/field_type.hpp"
#include "ecs/world.hpp"

using namespace sowa;

TEST_CASE("RenderOrder is a built-in, registered before any caller") {
	World world;

	const ComponentType type = world.FindComponent("RenderOrder");
	REQUIRE(type.IsValid());
	CHECK(world.RegisterComponent<RenderOrder>("RenderOrder").Error() == EcsError::DuplicateName);
}

TEST_CASE("RenderOrder describes its authored fields but not the derived ones") {
	World world;
	const ComponentType type = world.FindComponent("RenderOrder");
	REQUIRE(type.IsValid());

	REQUIRE(type.Fields().size() == 4);
	CHECK(type.FindField("zIndex") != nullptr);
	CHECK(type.FindField("zIndexMode") != nullptr);
	CHECK(type.FindField("layer") != nullptr);
	CHECK(type.FindField("layerMode") != nullptr);

	CHECK(type.FindField("effectiveZIndex") == nullptr);
	CHECK(type.FindField("effectiveLayer") == nullptr);
}

TEST_CASE("An entity with no RenderOrder is not disturbed by the pass") {
	World world;
	Entity entity = world.CreateEntity();

	world.Progress(0.f);

	CHECK_FALSE(entity.Has<RenderOrder>());
	// Absent means zero, so callers can treat "no component" as the default.
	CHECK(ComputeEffectiveZIndex(entity) == 0);
}

TEST_CASE("A root entity's effective z-index is its own value") {
	World world;
	Entity entity = world.CreateEntity();
	entity.Add(RenderOrder{5});

	world.Progress(0.f);

	CHECK(entity.Get<RenderOrder>()->effectiveZIndex == 5);
}

TEST_CASE("Relative mode adds the parent's effective z-index") {
	World world;

	Entity parent = world.CreateEntity();
	parent.Add(RenderOrder{10});
	Entity child = world.CreateEntity();
	child.Add(RenderOrder{3});
	REQUIRE(child.SetParent(parent));

	world.Progress(0.f);

	CHECK(parent.Get<RenderOrder>()->effectiveZIndex == 10);
	CHECK(child.Get<RenderOrder>()->effectiveZIndex == 13);
}

TEST_CASE("Relative offsets accumulate down a chain") {
	World world;

	Entity previous;
	std::vector<Entity> chain;
	for (i32 i = 0; i < 5; ++i) {
		Entity entity = world.CreateEntity();
		entity.Add(RenderOrder{2});
		if (previous.IsAlive())
			REQUIRE(entity.SetParent(previous));
		chain.push_back(entity);
		previous = entity;
	}

	world.Progress(0.f);

	for (i32 i = 0; i < 5; ++i)
		CHECK(chain[i].Get<RenderOrder>()->effectiveZIndex == 2 * (i + 1));
}

TEST_CASE("Absolute mode ignores every ancestor") {
	World world;

	Entity parent = world.CreateEntity();
	parent.Add(RenderOrder{100});
	Entity child = world.CreateEntity();
	child.Add(RenderOrder{7, ZIndexMode::Absolute});
	REQUIRE(child.SetParent(parent));

	world.Progress(0.f);

	CHECK(child.Get<RenderOrder>()->effectiveZIndex == 7);
}

TEST_CASE("An Absolute entity becomes the base for its relative descendants") {
	World world;

	Entity root = world.CreateEntity();
	root.Add(RenderOrder{1000});

	Entity ui = world.CreateEntity();
	ui.Add(RenderOrder{50, ZIndexMode::Absolute});
	REQUIRE(ui.SetParent(root));

	Entity button = world.CreateEntity();
	button.Add(RenderOrder{2});
	REQUIRE(button.SetParent(ui));

	Entity label = world.CreateEntity();
	label.Add(RenderOrder{1});
	REQUIRE(label.SetParent(button));

	world.Progress(0.f);

	// The whole UI subtree is pinned at 50 and stacks internally from there,
	// regardless of the 1000 above it.
	CHECK(ui.Get<RenderOrder>()->effectiveZIndex == 50);
	CHECK(button.Get<RenderOrder>()->effectiveZIndex == 52);
	CHECK(label.Get<RenderOrder>()->effectiveZIndex == 53);
}

TEST_CASE("An ancestor without a RenderOrder is skipped, not treated as zero-base") {
	World world;

	Entity grandparent = world.CreateEntity();
	grandparent.Add(RenderOrder{20});
	Entity grouping = world.CreateEntity(); // no RenderOrder of its own
	REQUIRE(grouping.SetParent(grandparent));
	Entity child = world.CreateEntity();
	child.Add(RenderOrder{5});
	REQUIRE(child.SetParent(grouping));

	world.Progress(0.f);

	CHECK(child.Get<RenderOrder>()->effectiveZIndex == 25);
}

TEST_CASE("Negative offsets push behind the parent") {
	World world;

	Entity parent = world.CreateEntity();
	parent.Add(RenderOrder{10});
	Entity shadow = world.CreateEntity();
	shadow.Add(RenderOrder{-1});
	REQUIRE(shadow.SetParent(parent));

	world.Progress(0.f);

	CHECK(shadow.Get<RenderOrder>()->effectiveZIndex == 9);
}

TEST_CASE("A chain whose tables are laid out child-first still settles in one pass") {
	World world;

	// Same adversarial construction as the transform suite: create every
	// entity unparented, then parent deepest-link-first, so table order is the
	// reverse of what the pass needs. Fails without cascade().
	constexpr i32 kDepth = 6;
	std::vector<Entity> chain;
	for (i32 i = 0; i < kDepth; ++i) {
		Entity entity = world.CreateEntity();
		entity.Add(RenderOrder{1});
		chain.push_back(entity);
	}
	for (i32 i = 0; i < kDepth - 1; ++i)
		REQUIRE(chain[i].SetParent(chain[i + 1]));

	world.Progress(0.f);

	// chain[kDepth - 1] is the root, chain[0] the deepest leaf.
	for (i32 i = 0; i < kDepth; ++i)
		CHECK(chain[i].Get<RenderOrder>()->effectiveZIndex == kDepth - i);
}

TEST_CASE("Changing a parent's z-index moves the whole subtree next frame") {
	World world;

	Entity parent = world.CreateEntity();
	parent.Add(RenderOrder{0});
	Entity child = world.CreateEntity();
	child.Add(RenderOrder{1});
	REQUIRE(child.SetParent(parent));

	world.Progress(0.f);
	REQUIRE(child.Get<RenderOrder>()->effectiveZIndex == 1);

	parent.Get<RenderOrder>()->zIndex = 500;
	world.Progress(0.f);
	CHECK(child.Get<RenderOrder>()->effectiveZIndex == 501);
}

TEST_CASE("Switching a child to Absolute detaches it from its parent's stack") {
	World world;

	Entity parent = world.CreateEntity();
	parent.Add(RenderOrder{100});
	Entity child = world.CreateEntity();
	child.Add(RenderOrder{5});
	REQUIRE(child.SetParent(parent));

	world.Progress(0.f);
	REQUIRE(child.Get<RenderOrder>()->effectiveZIndex == 105);

	child.Get<RenderOrder>()->zIndexMode = ZIndexMode::Absolute;
	world.Progress(0.f);
	CHECK(child.Get<RenderOrder>()->effectiveZIndex == 5);
}

TEST_CASE("ComputeEffectiveZIndex answers before the first Progress") {
	World world;

	Entity parent = world.CreateEntity();
	parent.Add(RenderOrder{10});
	Entity child = world.CreateEntity();
	child.Add(RenderOrder{3});
	REQUIRE(child.SetParent(parent));

	REQUIRE(child.Get<RenderOrder>()->effectiveZIndex == 0); // pass hasn't run
	CHECK(ComputeEffectiveZIndex(child) == 13);

	world.Progress(0.f);
	CHECK(child.Get<RenderOrder>()->effectiveZIndex == ComputeEffectiveZIndex(child));
}

TEST_CASE("ComputeEffectiveZIndex stops at an Absolute ancestor") {
	World world;

	Entity root = world.CreateEntity();
	root.Add(RenderOrder{1000});
	Entity ui = world.CreateEntity();
	ui.Add(RenderOrder{50, ZIndexMode::Absolute});
	REQUIRE(ui.SetParent(root));
	Entity button = world.CreateEntity();
	button.Add(RenderOrder{2});
	REQUIRE(button.SetParent(ui));

	CHECK(ComputeEffectiveZIndex(button) == 52);

	// Agrees with the cascade pass, which is the point of having both.
	world.Progress(0.f);
	CHECK(button.Get<RenderOrder>()->effectiveZIndex == 52);
}

TEST_CASE("ComputeEffectiveZIndex is zero for dead and null handles") {
	World world;

	Entity dead = world.CreateEntity();
	dead.Add(RenderOrder{9});
	dead.Destroy();

	CHECK(ComputeEffectiveZIndex(dead) == 0);
	CHECK(ComputeEffectiveZIndex(Entity()) == 0);
}

// Render layers.

TEST_CASE("Layer defaults to Inherit so a z-index-only RenderOrder changes nothing") {
	World world;

	Entity parent = world.CreateEntity();
	parent.Add(RenderOrder{0, ZIndexMode::Relative, 7, RenderLayerMode::Explicit});

	// Added purely to set a z-index. It must not silently fall out of its
	// parent's layer, which is why Inherit is the default.
	Entity child = world.CreateEntity();
	child.Add(RenderOrder{3});
	REQUIRE(child.SetParent(parent));

	world.Progress(0.f);

	CHECK(child.Get<RenderOrder>()->effectiveLayer == 7);
}

TEST_CASE("An explicit layer overrides the inherited one and passes down") {
	World world;

	Entity world0 = world.CreateEntity();
	world0.Add(RenderOrder{0, ZIndexMode::Relative, 0, RenderLayerMode::Explicit});

	Entity hud = world.CreateEntity();
	hud.Add(RenderOrder{0, ZIndexMode::Relative, 10, RenderLayerMode::Explicit});
	REQUIRE(hud.SetParent(world0));

	Entity label = world.CreateEntity();
	label.Add(RenderOrder{1});
	REQUIRE(label.SetParent(hud));

	world.Progress(0.f);

	CHECK(world0.Get<RenderOrder>()->effectiveLayer == 0);
	CHECK(hud.Get<RenderOrder>()->effectiveLayer == 10);
	CHECK(label.Get<RenderOrder>()->effectiveLayer == 10);
}

TEST_CASE("A layer beats any z-index below it") {
	World world;

	// The exact case the layer concept exists for: something with the lowest
	// z-index in the scene still draws above something with the highest,
	// because the sort key is (layer, z) and layer is the coarse pass.
	Entity ground = world.CreateEntity();
	ground.Add(RenderOrder{9999});

	Entity hud = world.CreateEntity();
	hud.Add(RenderOrder{-9999, ZIndexMode::Absolute, 1, RenderLayerMode::Explicit});

	world.Progress(0.f);

	const RenderOrder *groundOrder = ground.Get<RenderOrder>();
	const RenderOrder *hudOrder = hud.Get<RenderOrder>();
	REQUIRE(groundOrder->effectiveZIndex > hudOrder->effectiveZIndex);
	CHECK(hudOrder->effectiveLayer > groundOrder->effectiveLayer);
}

TEST_CASE("An explicit layer leaves the relative z chain alone") {
	World world;

	Entity parent = world.CreateEntity();
	parent.Add(RenderOrder{100});

	Entity child = world.CreateEntity();
	child.Add(RenderOrder{5, ZIndexMode::Relative, 2, RenderLayerMode::Explicit});
	REQUIRE(child.SetParent(parent));

	world.Progress(0.f);

	// Layer and z-index inherit independently - crossing to a new layer does
	// not reset the z accumulation. Harmless, since z is only ever compared
	// within a layer; ZIndexMode::Absolute is how you opt out.
	CHECK(child.Get<RenderOrder>()->effectiveLayer == 2);
	CHECK(child.Get<RenderOrder>()->effectiveZIndex == 105);
}

TEST_CASE("Absolute z and an explicit layer together fully detach a subtree") {
	World world;

	Entity parent = world.CreateEntity();
	parent.Add(RenderOrder{500, ZIndexMode::Relative, 3, RenderLayerMode::Explicit});

	Entity detached = world.CreateEntity();
	detached.Add(RenderOrder{0, ZIndexMode::Absolute, 9, RenderLayerMode::Explicit});
	REQUIRE(detached.SetParent(parent));

	world.Progress(0.f);

	CHECK(detached.Get<RenderOrder>()->effectiveZIndex == 0);
	CHECK(detached.Get<RenderOrder>()->effectiveLayer == 9);
}

TEST_CASE("An ancestor without a RenderOrder does not break layer inheritance") {
	World world;

	Entity layered = world.CreateEntity();
	layered.Add(RenderOrder{0, ZIndexMode::Relative, 4, RenderLayerMode::Explicit});
	Entity grouping = world.CreateEntity(); // no RenderOrder
	REQUIRE(grouping.SetParent(layered));
	Entity child = world.CreateEntity();
	child.Add(RenderOrder{0});
	REQUIRE(child.SetParent(grouping));

	world.Progress(0.f);

	CHECK(child.Get<RenderOrder>()->effectiveLayer == 4);
}

TEST_CASE("Changing a parent's layer moves the whole inheriting subtree") {
	World world;

	Entity parent = world.CreateEntity();
	parent.Add(RenderOrder{0, ZIndexMode::Relative, 1, RenderLayerMode::Explicit});
	Entity child = world.CreateEntity();
	child.Add(RenderOrder{0});
	REQUIRE(child.SetParent(parent));

	world.Progress(0.f);
	REQUIRE(child.Get<RenderOrder>()->effectiveLayer == 1);

	parent.Get<RenderOrder>()->layer = 42;
	world.Progress(0.f);
	CHECK(child.Get<RenderOrder>()->effectiveLayer == 42);
}

TEST_CASE("A layer value is ignored while the mode is Inherit") {
	World world;

	Entity parent = world.CreateEntity();
	parent.Add(RenderOrder{0, ZIndexMode::Relative, 5, RenderLayerMode::Explicit});

	Entity child = world.CreateEntity();
	// Has a layer value, but Inherit mode, so the value must not be used.
	child.Add(RenderOrder{0, ZIndexMode::Relative, 99, RenderLayerMode::Inherit});
	REQUIRE(child.SetParent(parent));

	world.Progress(0.f);
	CHECK(child.Get<RenderOrder>()->effectiveLayer == 5);

	// Flipping the mode makes the value take effect, with no other change.
	child.Get<RenderOrder>()->layerMode = RenderLayerMode::Explicit;
	world.Progress(0.f);
	CHECK(child.Get<RenderOrder>()->effectiveLayer == 99);
}

TEST_CASE("Layers settle in one pass on a child-first table layout too") {
	World world;

	constexpr i32 kDepth = 6;
	std::vector<Entity> chain;
	for (i32 i = 0; i < kDepth; ++i) {
		Entity entity = world.CreateEntity();
		entity.Add(RenderOrder{0});
		chain.push_back(entity);
	}
	// Root gets the explicit layer; everyone below inherits it.
	chain[kDepth - 1].Get<RenderOrder>()->layer = 8;
	chain[kDepth - 1].Get<RenderOrder>()->layerMode = RenderLayerMode::Explicit;

	for (i32 i = 0; i < kDepth - 1; ++i)
		REQUIRE(chain[i].SetParent(chain[i + 1]));

	world.Progress(0.f);

	for (i32 i = 0; i < kDepth; ++i)
		CHECK(chain[i].Get<RenderOrder>()->effectiveLayer == 8);
}

TEST_CASE("ComputeEffectiveLayer agrees with the pass, and answers before it") {
	World world;

	Entity root = world.CreateEntity();
	root.Add(RenderOrder{0, ZIndexMode::Relative, 2, RenderLayerMode::Explicit});
	Entity middle = world.CreateEntity();
	middle.Add(RenderOrder{0});
	REQUIRE(middle.SetParent(root));
	Entity leaf = world.CreateEntity();
	leaf.Add(RenderOrder{0});
	REQUIRE(leaf.SetParent(middle));

	REQUIRE(leaf.Get<RenderOrder>()->effectiveLayer == 0); // pass hasn't run
	CHECK(ComputeEffectiveLayer(leaf) == 2);

	world.Progress(0.f);
	CHECK(leaf.Get<RenderOrder>()->effectiveLayer == ComputeEffectiveLayer(leaf));
}

TEST_CASE("ComputeEffectiveLayer is zero with no explicit layer anywhere") {
	World world;

	Entity parent = world.CreateEntity();
	parent.Add(RenderOrder{0, ZIndexMode::Relative, 77, RenderLayerMode::Inherit});
	Entity child = world.CreateEntity();
	child.Add(RenderOrder{0});
	REQUIRE(child.SetParent(parent));

	CHECK(ComputeEffectiveLayer(child) == 0);

	Entity dead = world.CreateEntity();
	dead.Add(RenderOrder{0, ZIndexMode::Relative, 5, RenderLayerMode::Explicit});
	dead.Destroy();
	CHECK(ComputeEffectiveLayer(dead) == 0);
	CHECK(ComputeEffectiveLayer(Entity()) == 0);
	CHECK(ComputeEffectiveLayer(world.CreateEntity()) == 0);
}

// --- Sort key ---

TEST_CASE("The sort key orders by layer first, then z-index") {
	CHECK(RenderSortKey{0, 1} < RenderSortKey{0, 2});
	CHECK_FALSE(RenderSortKey{0, 2} < RenderSortKey{0, 1});

	// A higher layer wins regardless of z, in both directions.
	CHECK(RenderSortKey{0, 9999} < RenderSortKey{1, -9999});
	CHECK_FALSE(RenderSortKey{1, -9999} < RenderSortKey{0, 9999});

	// Equal keys are not ordered either way, which is what keeps a stable sort
	// stable rather than tripping a strict-weak-ordering assert.
	CHECK_FALSE(RenderSortKey{2, 3} < RenderSortKey{2, 3});

	// Negative layers sort below layer 0 - useful for a parallax backdrop.
	CHECK(RenderSortKey{-1, 500} < RenderSortKey{0, -500});
}

TEST_CASE("An entity with no RenderOrder sorts at the default key") {
	World world;

	Entity plain = world.CreateEntity();
	Entity ordered = world.CreateEntity();
	ordered.Add(RenderOrder{5});
	world.Progress(0.f);

	const RenderSortKey plainKey = GetRenderSortKey(plain.Get<RenderOrder>());
	CHECK(plainKey.layer == 0);
	CHECK(plainKey.zIndex == 0);
	CHECK(plainKey < GetRenderSortKey(ordered.Get<RenderOrder>()));
}

TEST_CASE("Sorting a scene by key puts a layered subtree on top") {
	World world;

	// Layer must take precedence over z-index.
	Entity ground = world.CreateEntity();
	ground.Add(RenderOrder{0});
	Entity prop = world.CreateEntity();
	prop.Add(RenderOrder{100});
	Entity hud = world.CreateEntity();
	hud.Add(RenderOrder{-500, ZIndexMode::Absolute, 1, RenderLayerMode::Explicit});

	world.Progress(0.f);

	std::vector<std::pair<RenderSortKey, Entity>> items;
	for (Entity entity : {hud, prop, ground})
		items.emplace_back(GetRenderSortKey(entity.Get<RenderOrder>()), entity);
	std::stable_sort(items.begin(), items.end(),
					 [](const auto &a, const auto &b) { return a.first < b.first; });

	CHECK(items[0].second == ground);
	CHECK(items[1].second == prop);
	CHECK(items[2].second == hud);
}

TEST_CASE("Render order is independent of the transform hierarchy's own values") {
	World world;

	// A subtree can be deep in the transform hierarchy and still sort first.
	Entity background = world.CreateEntity();
	background.Add(RenderOrder{0});
	Entity foreground = world.CreateEntity();
	foreground.Add(RenderOrder{10});
	REQUIRE(foreground.SetParent(background));

	Entity overlay = world.CreateEntity();
	overlay.Add(RenderOrder{-5, ZIndexMode::Absolute});
	REQUIRE(overlay.SetParent(foreground));

	world.Progress(0.f);

	CHECK(overlay.Get<RenderOrder>()->effectiveZIndex == -5);
	CHECK(foreground.Get<RenderOrder>()->effectiveZIndex == 10);
}
