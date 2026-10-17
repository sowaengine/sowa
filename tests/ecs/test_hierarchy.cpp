#include <string>
#include <vector>

#include <doctest/doctest.h>

#include "ecs/entity.hpp"
#include "ecs/world.hpp"
#include "math/vector2.hpp"

using namespace sowa;

namespace {

struct Position {
	Vector2 value;
};

} // namespace

TEST_CASE("SetParent links an entity into a parent's children") {
	World world;

	Entity parent = world.CreateEntity();
	Entity child = world.CreateEntity();

	CHECK_FALSE(child.HasParent());
	CHECK_FALSE(child.GetParent().IsAlive());

	CHECK(child.SetParent(parent));

	CHECK(child.HasParent());
	CHECK(child.GetParent() == parent);
	REQUIRE(parent.GetChildren().size() == 1);
	CHECK(parent.GetChildren()[0] == child);
}

TEST_CASE("Children iterate in insertion order") {
	World world;
	Entity parent = world.CreateEntity();

	std::vector<Entity> added;
	for (i32 i = 0; i < 8; ++i) {
		Entity child = world.CreateEntity();
		REQUIRE(child.SetParent(parent));
		added.push_back(child);
	}

	// Children must follow insertion order for scene serialization and the editor tree.
	const std::vector<Entity> children = parent.GetChildren();
	REQUIRE(children.size() == added.size());
	for (size_t i = 0; i < added.size(); ++i)
		CHECK(children[i] == added[i]);

	// EachChild agrees with GetChildren.
	std::vector<Entity> visited;
	parent.EachChild([&](Entity child) { visited.push_back(child); });
	CHECK(visited == added);
}

TEST_CASE("Child order survives components being added to children") {
	World world;
	REQUIRE(world.RegisterComponent<Position>("Position").Result());

	Entity parent = world.CreateEntity();
	std::vector<Entity> added;
	for (i32 i = 0; i < 6; ++i) {
		Entity child = world.CreateEntity();
		REQUIRE(child.SetParent(parent));
		added.push_back(child);
	}

	// Adding a component moves an entity between tables; order must not be
	// an accident of table layout.
	added[4].Add(Position{{1.f, 1.f}});
	added[1].Add(Position{{2.f, 2.f}});

	CHECK(parent.GetChildren() == added);
}

TEST_CASE("SetParent reparents, removing the child from its old parent") {
	World world;

	Entity first = world.CreateEntity();
	Entity second = world.CreateEntity();
	Entity child = world.CreateEntity();

	REQUIRE(child.SetParent(first));
	REQUIRE(child.SetParent(second));

	CHECK(child.GetParent() == second);
	CHECK(first.GetChildren().empty());
	REQUIRE(second.GetChildren().size() == 1);
	CHECK(second.GetChildren()[0] == child);
}

TEST_CASE("ClearParent makes an entity a root again") {
	World world;

	Entity parent = world.CreateEntity();
	Entity child = world.CreateEntity();
	REQUIRE(child.SetParent(parent));

	child.ClearParent();

	CHECK_FALSE(child.HasParent());
	CHECK(child.IsAlive());
	CHECK(parent.GetChildren().empty());

	// Clearing a root is a no-op, not an error.
	child.ClearParent();
	CHECK(child.IsAlive());
}

TEST_CASE("Destroying a parent destroys its whole subtree") {
	World world;

	Entity root = world.CreateEntity();
	Entity child = world.CreateEntity();
	Entity grandchild = world.CreateEntity();
	Entity unrelated = world.CreateEntity();

	REQUIRE(child.SetParent(root));
	REQUIRE(grandchild.SetParent(child));

	root.Destroy();

	CHECK_FALSE(root.IsAlive());
	CHECK_FALSE(child.IsAlive());
	CHECK_FALSE(grandchild.IsAlive());
	CHECK(unrelated.IsAlive());
}

TEST_CASE("IsDescendantOf walks the whole ancestor chain") {
	World world;

	Entity root = world.CreateEntity();
	Entity child = world.CreateEntity();
	Entity grandchild = world.CreateEntity();
	Entity sibling = world.CreateEntity();

	REQUIRE(child.SetParent(root));
	REQUIRE(grandchild.SetParent(child));
	REQUIRE(sibling.SetParent(root));

	CHECK(child.IsDescendantOf(root));
	CHECK(grandchild.IsDescendantOf(child));
	CHECK(grandchild.IsDescendantOf(root)); // transitive

	CHECK_FALSE(root.IsDescendantOf(grandchild));
	CHECK_FALSE(grandchild.IsDescendantOf(sibling));
	// An entity is not its own descendant.
	CHECK_FALSE(root.IsDescendantOf(root));
}

TEST_CASE("SetParent refuses to build a cycle") {
	World world;

	Entity root = world.CreateEntity();
	Entity child = world.CreateEntity();
	Entity grandchild = world.CreateEntity();
	REQUIRE(child.SetParent(root));
	REQUIRE(grandchild.SetParent(child));

	// The editor drag-drop mistake: drop a parent onto its own descendant.
	CHECK_FALSE(root.SetParent(grandchild));
	CHECK_FALSE(root.SetParent(child));
	CHECK_FALSE(root.SetParent(root));

	// The tree is untouched by the rejected attempts.
	CHECK_FALSE(root.HasParent());
	CHECK(child.GetParent() == root);
	CHECK(grandchild.GetParent() == child);
}

TEST_CASE("SetParent is safe on dead and null handles") {
	World world;

	Entity parent = world.CreateEntity();
	Entity child = world.CreateEntity();
	Entity dead = world.CreateEntity();
	dead.Destroy();

	CHECK_FALSE(child.SetParent(dead));
	CHECK_FALSE(dead.SetParent(parent));
	CHECK_FALSE(child.SetParent(Entity()));
	CHECK_FALSE(Entity().SetParent(parent));

	CHECK_FALSE(child.HasParent());

	Entity null;
	CHECK_FALSE(null.GetParent().IsAlive());
	CHECK(null.GetChildren().empty());
	CHECK_FALSE(null.IsDescendantOf(parent));
	null.ClearParent();
}

TEST_CASE("SetChildOrder reorders children") {
	World world;
	Entity parent = world.CreateEntity();

	std::vector<Entity> children;
	for (i32 i = 0; i < 4; ++i) {
		Entity child = world.CreateEntity();
		REQUIRE(child.SetParent(parent));
		children.push_back(child);
	}

	std::vector<Entity> reordered = {children[2], children[0], children[3], children[1]};
	CHECK(parent.SetChildOrder(reordered));
	CHECK(parent.GetChildren() == reordered);
}

TEST_CASE("SetChildOrder rejects a list that isn't exactly the children") {
	World world;
	Entity parent = world.CreateEntity();
	Entity outsider = world.CreateEntity();

	std::vector<Entity> children;
	for (i32 i = 0; i < 3; ++i) {
		Entity child = world.CreateEntity();
		REQUIRE(child.SetParent(parent));
		children.push_back(child);
	}
	const std::vector<Entity> original = parent.GetChildren();

	// Too short.
	CHECK_FALSE(parent.SetChildOrder({children[0], children[1]}));
	// Right count, but contains a non-child.
	CHECK_FALSE(parent.SetChildOrder({children[0], children[1], outsider}));
	// Right count, but a duplicate instead of the full set.
	CHECK_FALSE(parent.SetChildOrder({children[0], children[0], children[1]}));

	CHECK(parent.GetChildren() == original);
}

TEST_CASE("A deep chain parents and unwinds correctly") {
	World world;

	std::vector<Entity> chain;
	Entity previous;
	for (i32 i = 0; i < 32; ++i) {
		Entity entity = world.CreateEntity();
		if (previous.IsAlive())
			REQUIRE(entity.SetParent(previous));
		chain.push_back(entity);
		previous = entity;
	}

	CHECK(chain.back().IsDescendantOf(chain.front()));
	CHECK_FALSE(chain.front().IsDescendantOf(chain.back()));
	// Every link has exactly one child except the leaf.
	for (size_t i = 0; i + 1 < chain.size(); ++i) {
		REQUIRE(chain[i].GetChildren().size() == 1);
		CHECK(chain[i].GetChildren()[0] == chain[i + 1]);
	}
	CHECK(chain.back().GetChildren().empty());

	chain.front().Destroy();
	for (Entity &entity : chain)
		CHECK_FALSE(entity.IsAlive());
}

TEST_CASE("Hierarchy and components are independent") {
	World world;
	REQUIRE(world.RegisterComponent<Position>("Position").Result());

	Entity parent = world.CreateEntity();
	parent.Add(Position{{1.f, 2.f}});
	Entity child = world.CreateEntity();
	child.Add(Position{{3.f, 4.f}});
	REQUIRE(child.SetParent(parent));

	// Parenting doesn't disturb component data on either side.
	CHECK(parent.Get<Position>()->value.x == doctest::Approx(1.f));
	CHECK(child.Get<Position>()->value.x == doctest::Approx(3.f));

	// Both are still matched by an ordinary query.
	CHECK(world.CreateQuery<Position>().Count() == 2);
}

// flecs parent() returns a zero ID with a live world for roots. Calling is_alive()
// on that handle aborts; GetParent must normalize it to an empty Entity.
TEST_CASE("GetParent on a root yields a safe null handle") {
	World world;
	Entity root = world.CreateEntity();

	const Entity parent = root.GetParent();
	CHECK_FALSE(parent.IsAlive()); // aborted before the fix
	CHECK(parent == Entity());
	CHECK_FALSE(root.HasParent());

	// The null handle stays inert for everything else too.
	CHECK(parent.GetChildren().empty());
	CHECK_FALSE(parent.IsDescendantOf(root));
	CHECK_FALSE(root.IsDescendantOf(parent));
}
