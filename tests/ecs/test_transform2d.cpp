#include <doctest/doctest.h>

#include <glm/glm.hpp>

#include "ecs/components/transform2d.hpp"
#include "ecs/entity.hpp"
#include "ecs/world.hpp"
#include "math/vector2.hpp"

using namespace sowa;

namespace {

// Transforms compose through several matrix multiplies, so exact equality is
// the wrong bar; this is loose enough for accumulated float error and tight
// enough that a wrong composition order still fails.
constexpr f32 kEpsilon = 0.001f;

void CheckVector2(Vector2 actual, Vector2 expected) {
	CHECK(actual.x == doctest::Approx(expected.x).epsilon(kEpsilon));
	CHECK(actual.y == doctest::Approx(expected.y).epsilon(kEpsilon));
}

} // namespace

TEST_CASE("Transform2D is a built-in component, registered before any caller") {
	World world;

	const ComponentType type = world.FindComponent("Transform2D");
	REQUIRE(type.IsValid());

	// Registering it again must fail, the same as any duplicate: it is the
	// engine's component, not something a caller owns.
	CHECK(world.RegisterComponent<Transform2D>("Transform2D").Error() == EcsError::DuplicateName);
}

TEST_CASE("Transform2D exposes its authored fields but not the derived matrix") {
	World world;
	const ComponentType type = world.FindComponent("Transform2D");
	REQUIRE(type.IsValid());

	REQUIRE(type.Fields().size() == 3);
	CHECK(type.FindField("position") != nullptr);
	CHECK(type.FindField("rotation") != nullptr);
	CHECK(type.FindField("scale") != nullptr);

	CHECK(type.FindField("worldMatrix") == nullptr);
}

TEST_CASE("Transform2D defaults to identity") {
	Transform2D transform;

	CheckVector2(transform.position, Vector2(0.f, 0.f));
	CHECK(transform.rotation == doctest::Approx(0.f));
	CheckVector2(transform.scale, Vector2(1.f, 1.f));
	CHECK(transform.LocalMatrix() == glm::mat4(1.f));
	CHECK(transform.worldMatrix == glm::mat4(1.f));
}

TEST_CASE("Add<Transform2D>() default-constructs rather than zeroing") {
	World world;

	Entity entity = world.CreateEntity();
	entity.Add<Transform2D>();

	// Nastier than it looks: scale defaults to (1,1) via a default member
	// initializer, so if flecs ever relocated to a memset-style default the
	// component would come out with zero scale and everything holding one
	// would silently render at no size at all.
	const Transform2D *transform = entity.Get<Transform2D>();
	REQUIRE(transform != nullptr);
	CheckVector2(transform->scale, Vector2(1.f, 1.f));
	CheckVector2(transform->position, Vector2(0.f, 0.f));
	CHECK(transform->rotation == doctest::Approx(0.f));

	world.Progress(0.f);
	CHECK(entity.Get<Transform2D>()->worldMatrix == glm::mat4(1.f));
}

TEST_CASE("A root entity's world transform is just its local transform") {
	World world;

	Entity entity = world.CreateEntity();
	entity.Add(Transform2D{Vector2(10.f, 20.f), 90.f, Vector2(2.f, 2.f)});

	world.Progress(0.f);

	const Transform2D *transform = entity.Get<Transform2D>();
	REQUIRE(transform != nullptr);
	CheckVector2(transform->WorldPosition(), Vector2(10.f, 20.f));
	CHECK(transform->WorldRotation() == doctest::Approx(90.f).epsilon(kEpsilon));
	CheckVector2(transform->WorldScale(), Vector2(2.f, 2.f));
	CHECK(transform->worldMatrix == transform->LocalMatrix());
}

TEST_CASE("CalculateTransforms still runs when play-only systems are off") {
	World world;
	Entity entity = world.CreateEntity();
	entity.Add(Transform2D{Vector2(10.f, 20.f)});

	world.SetPlayOnlySystemsEnabled(false);
	world.Progress(0.f);

	CheckVector2(entity.Get<Transform2D>()->WorldPosition(), Vector2(10.f, 20.f));
}

TEST_CASE("A child's world position is offset by its parent") {
	World world;

	Entity parent = world.CreateEntity();
	parent.Add(Transform2D{Vector2(100.f, 50.f)});

	Entity child = world.CreateEntity();
	child.Add(Transform2D{Vector2(10.f, 5.f)});
	REQUIRE(child.SetParent(parent));

	world.Progress(0.f);

	CheckVector2(child.Get<Transform2D>()->WorldPosition(), Vector2(110.f, 55.f));
	// The local values are untouched by the pass - only worldMatrix is derived.
	CheckVector2(child.Get<Transform2D>()->position, Vector2(10.f, 5.f));
}

TEST_CASE("A parent's rotation orbits its children") {
	World world;

	Entity parent = world.CreateEntity();
	parent.Add(Transform2D{Vector2(0.f, 0.f), 90.f, Vector2(1.f, 1.f)});

	Entity child = world.CreateEntity();
	child.Add(Transform2D{Vector2(10.f, 0.f)});
	REQUIRE(child.SetParent(parent));

	world.Progress(0.f);

	// +90 degrees is counter-clockwise from +X, so a child out along the
	// parent's +X axis swings to +Y. A sign flip here is exactly the old
	// engine's rotation wart that the convention decision fixed.
	CheckVector2(child.Get<Transform2D>()->WorldPosition(), Vector2(0.f, 10.f));
	CHECK(child.Get<Transform2D>()->WorldRotation() == doctest::Approx(90.f).epsilon(kEpsilon));
}

TEST_CASE("A parent's scale multiplies its children's offset and scale") {
	World world;

	Entity parent = world.CreateEntity();
	parent.Add(Transform2D{Vector2(0.f, 0.f), 0.f, Vector2(2.f, 3.f)});

	Entity child = world.CreateEntity();
	child.Add(Transform2D{Vector2(10.f, 10.f), 0.f, Vector2(4.f, 4.f)});
	REQUIRE(child.SetParent(parent));

	world.Progress(0.f);

	CheckVector2(child.Get<Transform2D>()->WorldPosition(), Vector2(20.f, 30.f));
	CheckVector2(child.Get<Transform2D>()->WorldScale(), Vector2(8.f, 12.f));
}

TEST_CASE("World transforms compose down a deep chain in one pass") {
	World world;

	Entity previous;
	std::vector<Entity> chain;
	for (i32 i = 0; i < 6; ++i) {
		Entity entity = world.CreateEntity();
		entity.Add(Transform2D{Vector2(10.f, 0.f)});
		if (previous.IsAlive())
			REQUIRE(entity.SetParent(previous));
		chain.push_back(entity);
		previous = entity;
	}

	world.Progress(0.f);

	// Each level adds 10 along +X. If cascade() handed back children before
	// parents, the deeper levels would be composing against a stale matrix and
	// this would need several frames to settle.
	for (size_t i = 0; i < chain.size(); ++i) {
		CheckVector2(chain[i].Get<Transform2D>()->WorldPosition(),
					 Vector2(10.f * static_cast<f32>(i + 1), 0.f));
	}
}

TEST_CASE("A chain whose tables are laid out child-first still settles in one pass") {
	World world;

	// Register child tables before parent tables to require cascade ordering.
	constexpr i32 kDepth = 6;
	std::vector<Entity> chain;
	for (i32 i = 0; i < kDepth; ++i) {
		Entity entity = world.CreateEntity();
		entity.Add(Transform2D{Vector2(10.f, 0.f)});
		chain.push_back(entity);
	}
	for (i32 i = 0; i < kDepth - 1; ++i)
		REQUIRE(chain[i].SetParent(chain[i + 1]));

	world.Progress(0.f);

	// chain[kDepth - 1] is the root, chain[0] the deepest leaf.
	for (i32 i = 0; i < kDepth; ++i) {
		const f32 depthFromRoot = static_cast<f32>(kDepth - i);
		CheckVector2(chain[i].Get<Transform2D>()->WorldPosition(),
					 Vector2(10.f * depthFromRoot, 0.f));
	}
}

TEST_CASE("A deep chain settles in a single Progress, not over several frames") {
	World world;

	Entity root = world.CreateEntity();
	root.Add(Transform2D{Vector2(1.f, 0.f)});
	Entity child = world.CreateEntity();
	child.Add(Transform2D{Vector2(1.f, 0.f)});
	REQUIRE(child.SetParent(root));
	Entity grandchild = world.CreateEntity();
	grandchild.Add(Transform2D{Vector2(1.f, 0.f)});
	REQUIRE(grandchild.SetParent(child));

	world.Progress(0.f);
	CheckVector2(grandchild.Get<Transform2D>()->WorldPosition(), Vector2(3.f, 0.f));

	// Moving the root propagates all the way down on the very next frame.
	root.Get<Transform2D>()->position = Vector2(100.f, 0.f);
	world.Progress(0.f);
	CheckVector2(grandchild.Get<Transform2D>()->WorldPosition(), Vector2(102.f, 0.f));
}

TEST_CASE("An ancestor without a Transform2D is skipped, not treated as a root") {
	World world;

	Entity grandparent = world.CreateEntity();
	grandparent.Add(Transform2D{Vector2(100.f, 0.f)});

	// A plain grouping entity in the middle: no transform of its own.
	Entity grouping = world.CreateEntity();
	REQUIRE(grouping.SetParent(grandparent));

	Entity child = world.CreateEntity();
	child.Add(Transform2D{Vector2(5.f, 0.f)});
	REQUIRE(child.SetParent(grouping));

	world.Progress(0.f);

	CheckVector2(child.Get<Transform2D>()->WorldPosition(), Vector2(105.f, 0.f));
}

TEST_CASE("Detaching from a parent drops that parent's contribution") {
	World world;

	Entity parent = world.CreateEntity();
	parent.Add(Transform2D{Vector2(100.f, 100.f)});
	Entity child = world.CreateEntity();
	child.Add(Transform2D{Vector2(10.f, 10.f)});
	REQUIRE(child.SetParent(parent));

	world.Progress(0.f);
	CheckVector2(child.Get<Transform2D>()->WorldPosition(), Vector2(110.f, 110.f));

	child.ClearParent();
	world.Progress(0.f);

	// SetParent/ClearParent do not preserve the world transform - that is what
	// Reparent is for. The child snaps back to its own local values.
	CheckVector2(child.Get<Transform2D>()->WorldPosition(), Vector2(10.f, 10.f));
}

TEST_CASE("ComputeWorldMatrix answers before the first Progress has run") {
	World world;

	Entity parent = world.CreateEntity();
	parent.Add(Transform2D{Vector2(100.f, 50.f)});
	Entity child = world.CreateEntity();
	child.Add(Transform2D{Vector2(10.f, 5.f)});
	REQUIRE(child.SetParent(parent));

	// The cached matrix is still identity - no pass has run yet.
	REQUIRE(child.Get<Transform2D>()->worldMatrix == glm::mat4(1.f));

	const glm::mat4 computed = ComputeWorldMatrix(child);
	CHECK(computed[3][0] == doctest::Approx(110.f).epsilon(kEpsilon));
	CHECK(computed[3][1] == doctest::Approx(55.f).epsilon(kEpsilon));

	// And it agrees with what the pass produces once it runs.
	world.Progress(0.f);
	CHECK(child.Get<Transform2D>()->worldMatrix == computed);
}

TEST_CASE("ComputeWorldMatrix is identity for dead and transformless entities") {
	World world;

	Entity dead = world.CreateEntity();
	dead.Add(Transform2D{Vector2(10.f, 10.f)});
	dead.Destroy();
	CHECK(ComputeWorldMatrix(dead) == glm::mat4(1.f));

	CHECK(ComputeWorldMatrix(Entity()) == glm::mat4(1.f));
	CHECK(ComputeWorldMatrix(world.CreateEntity()) == glm::mat4(1.f));
}

TEST_CASE("Reparent keeps the child visually where it was") {
	World world;

	Entity oldParent = world.CreateEntity();
	oldParent.Add(Transform2D{Vector2(100.f, 0.f)});
	Entity newParent = world.CreateEntity();
	newParent.Add(Transform2D{Vector2(300.f, 40.f)});

	Entity child = world.CreateEntity();
	child.Add(Transform2D{Vector2(10.f, 5.f)});
	REQUIRE(child.SetParent(oldParent));

	world.Progress(0.f);
	REQUIRE(child.Get<Transform2D>()->WorldPosition().x == doctest::Approx(110.f).epsilon(kEpsilon));

	REQUIRE(Reparent(child, newParent));

	// The local values were rewritten to absorb the change of parent...
	CheckVector2(child.Get<Transform2D>()->position, Vector2(-190.f, -35.f));
	CHECK(child.GetParent() == newParent);

	// ...so the world position is unchanged, both immediately and after the
	// next pass confirms it.
	CheckVector2(child.Get<Transform2D>()->WorldPosition(), Vector2(110.f, 5.f));
	world.Progress(0.f);
	CheckVector2(child.Get<Transform2D>()->WorldPosition(), Vector2(110.f, 5.f));
}

TEST_CASE("Reparent preserves rotation and scale, not just position") {
	World world;

	Entity oldParent = world.CreateEntity();
	oldParent.Add(Transform2D{Vector2(0.f, 0.f), 90.f, Vector2(2.f, 2.f)});
	Entity newParent = world.CreateEntity();
	newParent.Add(Transform2D{Vector2(10.f, 10.f), -45.f, Vector2(0.5f, 0.5f)});

	Entity child = world.CreateEntity();
	child.Add(Transform2D{Vector2(20.f, 0.f), 30.f, Vector2(3.f, 3.f)});
	REQUIRE(child.SetParent(oldParent));
	world.Progress(0.f);

	const Vector2 before = child.Get<Transform2D>()->WorldPosition();
	const f32 rotationBefore = child.Get<Transform2D>()->WorldRotation();
	const Vector2 scaleBefore = child.Get<Transform2D>()->WorldScale();

	REQUIRE(Reparent(child, newParent));
	world.Progress(0.f);

	CheckVector2(child.Get<Transform2D>()->WorldPosition(), before);
	CHECK(child.Get<Transform2D>()->WorldRotation() == doctest::Approx(rotationBefore).epsilon(kEpsilon));
	CheckVector2(child.Get<Transform2D>()->WorldScale(), scaleBefore);
}

TEST_CASE("Reparent to a null parent detaches to the root, still in place") {
	World world;

	Entity parent = world.CreateEntity();
	parent.Add(Transform2D{Vector2(100.f, 100.f), 45.f, Vector2(2.f, 2.f)});
	Entity child = world.CreateEntity();
	child.Add(Transform2D{Vector2(10.f, 0.f)});
	REQUIRE(child.SetParent(parent));
	world.Progress(0.f);

	const Vector2 before = child.Get<Transform2D>()->WorldPosition();

	REQUIRE(Reparent(child, Entity()));
	CHECK_FALSE(child.HasParent());

	world.Progress(0.f);
	CheckVector2(child.Get<Transform2D>()->WorldPosition(), before);
	// Detached to the root, local and world now agree.
	CheckVector2(child.Get<Transform2D>()->position, before);
}

TEST_CASE("Reparent obeys the same cycle and liveness rules as SetParent") {
	World world;

	Entity parent = world.CreateEntity();
	parent.Add(Transform2D{});
	Entity child = world.CreateEntity();
	child.Add(Transform2D{});
	REQUIRE(child.SetParent(parent));

	CHECK_FALSE(Reparent(parent, parent));    // self
	CHECK_FALSE(Reparent(parent, child));     // into own subtree
	CHECK_FALSE(Reparent(Entity(), parent));  // null child

	Entity dead = world.CreateEntity();
	dead.Destroy();
	CHECK_FALSE(Reparent(dead, parent));

	// The hierarchy is untouched by every rejected call.
	CHECK(child.GetParent() == parent);
	CHECK_FALSE(parent.HasParent());
}

TEST_CASE("Reparent works on an entity with no Transform2D") {
	World world;

	Entity parent = world.CreateEntity();
	Entity child = world.CreateEntity();

	CHECK(Reparent(child, parent));
	CHECK(child.GetParent() == parent);

	CHECK(Reparent(child, Entity()));
	CHECK_FALSE(child.HasParent());
}

TEST_CASE("Transforms are recalculated after user systems have run") {
	World world;

	Entity entity = world.CreateEntity();
	entity.Add(Transform2D{Vector2(0.f, 0.f)});

	world.RegisterSystem<Transform2D>("Move", [](Entity, Transform2D &transform) {
		transform.position.x += 10.f;
	});

	world.Progress(0.f);

	// If CalculateTransforms ran in the same phase in registration order it
	// would have gone first, and the world matrix would trail the local values
	// by a frame.
	CheckVector2(entity.Get<Transform2D>()->WorldPosition(), Vector2(10.f, 0.f));
}

TEST_CASE("Destroying a parent takes its children's transforms with it") {
	World world;

	Entity parent = world.CreateEntity();
	parent.Add(Transform2D{Vector2(100.f, 0.f)});
	Entity child = world.CreateEntity();
	child.Add(Transform2D{Vector2(10.f, 0.f)});
	REQUIRE(child.SetParent(parent));
	world.Progress(0.f);

	parent.Destroy();
	world.Progress(0.f);

	// ChildOf's delete policy takes the subtree; the pass must not trip over
	// the entities disappearing underneath it.
	CHECK_FALSE(child.IsAlive());
}
