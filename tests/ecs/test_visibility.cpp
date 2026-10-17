#include <algorithm>
#include <vector>

#include <doctest/doctest.h>

#include "ecs/components/transform2d.hpp"
#include "ecs/entity.hpp"
#include "ecs/visibility.hpp"
#include "ecs/world.hpp"

using namespace sowa;

namespace {

std::vector<Entity> Visible(Query<Transform2D> &query) {
	std::vector<Entity> found;
	query.Each([&](Entity entity, Transform2D &) { found.push_back(entity); });
	return found;
}

bool Contains(const std::vector<Entity> &entities, Entity entity) {
	return std::find(entities.begin(), entities.end(), entity) != entities.end();
}

} // namespace

TEST_CASE("Entities are visible by default, with no component needed") {
	World world;
	Entity entity = world.CreateEntity();

	// Absence *is* visible - nothing to add, so a camera or physics body
	// carries no visibility state at all.
	CHECK(entity.IsVisible());
	CHECK(entity.IsVisibleInTree());
	CHECK_FALSE(entity.Has<Hidden>());
}

TEST_CASE("SetVisible toggles the hidden tag") {
	World world;
	Entity entity = world.CreateEntity();

	entity.SetVisible(false);
	CHECK_FALSE(entity.IsVisible());
	CHECK(entity.Has<Hidden>());

	entity.SetVisible(true);
	CHECK(entity.IsVisible());
	CHECK_FALSE(entity.Has<Hidden>());

	// Idempotent in both directions.
	entity.SetVisible(true);
	CHECK(entity.IsVisible());
	entity.SetVisible(false);
	entity.SetVisible(false);
	CHECK_FALSE(entity.IsVisible());
}

TEST_CASE("IsVisible is the entity's own flag, IsVisibleInTree the inherited one") {
	World world;

	Entity parent = world.CreateEntity();
	Entity child = world.CreateEntity();
	REQUIRE(child.SetParent(parent));

	parent.SetVisible(false);

	CHECK(child.IsVisible());
	CHECK_FALSE(child.IsVisibleInTree());
	CHECK_FALSE(parent.IsVisible());
	CHECK_FALSE(parent.IsVisibleInTree());
}

TEST_CASE("Hiding covers the whole subtree with no component on the children") {
	World world;

	Entity panel = world.CreateEntity();
	Entity button = world.CreateEntity();
	Entity label = world.CreateEntity();
	REQUIRE(button.SetParent(panel));
	REQUIRE(label.SetParent(button));

	panel.SetVisible(false);

	// The exact case a derived-flag design gets wrong: neither descendant has
	// any visibility state, and both must still be hidden. Otherwise hiding a
	// UI panel would require tagging every entity inside it.
	CHECK_FALSE(button.IsVisibleInTree());
	CHECK_FALSE(label.IsVisibleInTree());
	CHECK_FALSE(button.Has<Hidden>());
	CHECK_FALSE(label.Has<Hidden>());
}

TEST_CASE("A VisibleOnly query excludes hidden entities and their subtrees") {
	World world;

	Entity panel = world.CreateEntity();
	Entity button = world.CreateEntity();
	Entity loose = world.CreateEntity();
	panel.Add(Transform2D{});
	button.Add(Transform2D{});
	loose.Add(Transform2D{});
	REQUIRE(button.SetParent(panel));

	Query<Transform2D> visible = world.CreateQuery<Transform2D>(VisibilityFilter::VisibleOnly);
	Query<Transform2D> all = world.CreateQuery<Transform2D>();

	REQUIRE(Visible(visible).size() == 3);

	panel.SetVisible(false);
	const std::vector<Entity> remaining = Visible(visible);
	CHECK(remaining.size() == 1);
	CHECK(Contains(remaining, loose));
	CHECK_FALSE(Contains(remaining, panel));
	CHECK_FALSE(Contains(remaining, button));

	// The unfiltered query is unaffected - most queries have no business
	// caring about visibility.
	CHECK(all.Count() == 3);
}

TEST_CASE("Showing again restores the whole subtree to a cached query") {
	World world;

	Entity parent = world.CreateEntity();
	Entity child = world.CreateEntity();
	parent.Add(Transform2D{});
	child.Add(Transform2D{});
	REQUIRE(child.SetParent(parent));

	Query<Transform2D> visible = world.CreateQuery<Transform2D>(VisibilityFilter::VisibleOnly);

	parent.SetVisible(false);
	REQUIRE(Visible(visible).empty());

	parent.SetVisible(true);
	CHECK(Visible(visible).size() == 2);
}

TEST_CASE("Hiding is inherited from any depth, not just the immediate parent") {
	World world;

	constexpr i32 kDepth = 5;
	std::vector<Entity> chain;
	Entity previous;
	for (i32 i = 0; i < kDepth; ++i) {
		Entity entity = world.CreateEntity();
		entity.Add(Transform2D{});
		if (previous.IsAlive())
			REQUIRE(entity.SetParent(previous));
		chain.push_back(entity);
		previous = entity;
	}

	Query<Transform2D> visible = world.CreateQuery<Transform2D>(VisibilityFilter::VisibleOnly);
	REQUIRE(Visible(visible).size() == kDepth);

	// Hiding the root must remove the deepest leaf too - the ancestor term
	// walks the whole chain, it isn't a one-level check.
	chain[0].SetVisible(false);
	CHECK(Visible(visible).empty());
	CHECK_FALSE(chain[kDepth - 1].IsVisibleInTree());

	// Hiding a middle link splits the chain.
	chain[0].SetVisible(true);
	chain[2].SetVisible(false);
	const std::vector<Entity> remaining = Visible(visible);
	CHECK(remaining.size() == 2);
	CHECK(Contains(remaining, chain[0]));
	CHECK(Contains(remaining, chain[1]));
}

TEST_CASE("Reparenting under a hidden entity hides immediately") {
	World world;

	Entity hiddenParent = world.CreateEntity();
	Entity visibleParent = world.CreateEntity();
	Entity wanderer = world.CreateEntity();
	hiddenParent.Add(Transform2D{});
	visibleParent.Add(Transform2D{});
	wanderer.Add(Transform2D{});
	hiddenParent.SetVisible(false);

	Query<Transform2D> visible = world.CreateQuery<Transform2D>(VisibilityFilter::VisibleOnly);
	REQUIRE(Contains(Visible(visible), wanderer));

	// No pass runs in between - the query has to reflect this straight away,
	// including for a query cached before the reparent happened.
	REQUIRE(wanderer.SetParent(hiddenParent));
	CHECK_FALSE(Contains(Visible(visible), wanderer));
	CHECK_FALSE(wanderer.IsVisibleInTree());

	REQUIRE(wanderer.SetParent(visibleParent));
	CHECK(Contains(Visible(visible), wanderer));
	CHECK(wanderer.IsVisibleInTree());
}

TEST_CASE("An entity created under a hidden parent is hidden from the start") {
	World world;

	Entity panel = world.CreateEntity();
	panel.Add(Transform2D{});
	panel.SetVisible(false);

	Query<Transform2D> visible = world.CreateQuery<Transform2D>(VisibilityFilter::VisibleOnly);

	// Created after the query was cached, which is the case a stale cache
	// would get wrong.
	Entity spawned = world.CreateEntity();
	spawned.Add(Transform2D{});
	REQUIRE(spawned.SetParent(panel));

	CHECK(Visible(visible).empty());
	CHECK_FALSE(spawned.IsVisibleInTree());
}

TEST_CASE("A visible child of a hidden parent stays hidden until the parent shows") {
	World world;

	Entity parent = world.CreateEntity();
	Entity child = world.CreateEntity();
	parent.Add(Transform2D{});
	child.Add(Transform2D{});
	REQUIRE(child.SetParent(parent));

	parent.SetVisible(false);
	child.SetVisible(true); // explicitly visible, but the parent still wins

	Query<Transform2D> visible = world.CreateQuery<Transform2D>(VisibilityFilter::VisibleOnly);
	CHECK(Visible(visible).empty());

	parent.SetVisible(true);
	CHECK(Visible(visible).size() == 2);
}

TEST_CASE("A hidden child stays hidden when its parent is shown") {
	World world;

	Entity parent = world.CreateEntity();
	Entity hiddenChild = world.CreateEntity();
	Entity otherChild = world.CreateEntity();
	parent.Add(Transform2D{});
	hiddenChild.Add(Transform2D{});
	otherChild.Add(Transform2D{});
	REQUIRE(hiddenChild.SetParent(parent));
	REQUIRE(otherChild.SetParent(parent));

	hiddenChild.SetVisible(false);
	parent.SetVisible(false);
	parent.SetVisible(true);

	// Showing the parent must not clear the child's own flag.
	Query<Transform2D> visible = world.CreateQuery<Transform2D>(VisibilityFilter::VisibleOnly);
	const std::vector<Entity> found = Visible(visible);
	CHECK(found.size() == 2);
	CHECK_FALSE(Contains(found, hiddenChild));
	CHECK(hiddenChild.Has<Hidden>());
}

TEST_CASE("A VisibleOnly system skips hidden entities") {
	World world;

	Entity visible = world.CreateEntity();
	Entity hidden = world.CreateEntity();
	Entity childOfHidden = world.CreateEntity();
	visible.Add(Transform2D{});
	hidden.Add(Transform2D{});
	childOfHidden.Add(Transform2D{});
	REQUIRE(childOfHidden.SetParent(hidden));
	hidden.SetVisible(false);

	std::vector<Entity> ticked;
	world.CreateSystem<Transform2D>("Draw")
		.VisibleOnly()
		.Each([&](Entity entity, Transform2D &) { ticked.push_back(entity); });

	world.Progress(0.f);

	REQUIRE(ticked.size() == 1);
	CHECK(ticked[0] == visible);
}

TEST_CASE("Hidden entities still run ordinary systems") {
	World world;

	Entity entity = world.CreateEntity();
	entity.Add(Transform2D{});
	entity.SetVisible(false);

	i32 ticks = 0;
	world.RegisterSystem<Transform2D>("Move", [&](Entity, Transform2D &) { ++ticks; });
	world.Progress(0.f);

	// Visibility is a rendering concern only - it does not disable simulation.
	CHECK(ticks == 1);
}

TEST_CASE("Hidden entities still get their transforms calculated") {
	World world;

	Entity parent = world.CreateEntity();
	parent.Add(Transform2D{Vector2(100.f, 0.f)});
	Entity child = world.CreateEntity();
	child.Add(Transform2D{Vector2(10.f, 0.f)});
	REQUIRE(child.SetParent(parent));
	parent.SetVisible(false);

	world.Progress(0.f);

	// Otherwise a subtree would be stale the frame it becomes visible again.
	CHECK(child.Get<Transform2D>()->WorldPosition().x == doctest::Approx(110.f));
}

TEST_CASE("Visibility is safe on dead and null handles") {
	World world;

	Entity dead = world.CreateEntity();
	dead.SetVisible(false);
	dead.Destroy();

	dead.SetVisible(true);
	CHECK_FALSE(dead.IsVisible());
	CHECK_FALSE(dead.IsVisibleInTree());

	Entity null;
	null.SetVisible(false);
	CHECK_FALSE(null.IsVisible());
	CHECK_FALSE(null.IsVisibleInTree());
}
