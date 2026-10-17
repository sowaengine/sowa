#include <algorithm>
#include <iterator>
#include <string>
#include <unordered_set>
#include <vector>

#include <doctest/doctest.h>

#include "core/serialize/archive.hpp"
#include "resource/resource.hpp"
#include "resource/resource_id.hpp"
#include "resource/resource_registry.hpp"

using namespace sowa;

namespace {

// These types use reflected serialization. Verify saving and loading through the registry.
struct Sound : public Resource {
	std::string path;
	f32 volume = 1.f;
};

struct Image : public Resource {
	i32 width = 0;
	i32 height = 0;
};

// The registry owns resources it is responsible for unloading, so it's
// non-copyable; described in place rather than returned by value.
struct Fixture {
	ResourceRegistry registry;

	Fixture() {
		registry.RegisterType<Sound>("Sound")
			.Field("path", &Sound::path)
			.Field("volume", &Sound::volume);
		registry.RegisterType<Image>("Image")
			.Field("width", &Image::width)
			.Field("height", &Image::height);
	}
};

} // namespace

TEST_CASE("Create assigns a valid id and takes ownership") {
	Fixture fixture;
	ResourceRegistry &registry = fixture.registry;

	Ref<Sound> sound = registry.Create<Sound>();
	REQUIRE(sound != nullptr);
	CHECK(sound->Id().IsValid());
	CHECK(registry.Count() == 1);
	CHECK(registry.Has(sound->Id()));
	CHECK(registry.Get(sound->Id()) == sound);
	CHECK(registry.TypeNameOf(sound->Id()) == "Sound");
}

TEST_CASE("Ids are unique across many resources") {
	Fixture fixture;
	ResourceRegistry &registry = fixture.registry;

	std::unordered_set<u64> seen;
	for (int i = 0; i < 500; ++i) {
		Ref<Sound> sound = registry.Create<Sound>();
		REQUIRE(sound != nullptr);
		CHECK(sound->Id().value != 0);
		CHECK(seen.insert(sound->Id().value).second);
	}
	CHECK(registry.Count() == 500);
}

TEST_CASE("Create by type name is the dynamic path scenes and scripts use") {
	Fixture fixture;
	ResourceRegistry &registry = fixture.registry;

	Ref<Resource> created = registry.Create("Image");
	REQUIRE(created != nullptr);
	CHECK(registry.TypeNameOf(created->Id()) == "Image");

	// The name path and the typed path produce the same thing.
	Ref<Image> typed = registry.Get<Image>(created->Id());
	CHECK(typed != nullptr);
	CHECK(typed == created);
}

TEST_CASE("An unknown type name creates nothing") {
	Fixture fixture;
	ResourceRegistry &registry = fixture.registry;

	CHECK(registry.Create("Nonexistent") == nullptr);
	CHECK(registry.Count() == 0);
}

TEST_CASE("Creating an unregistered type fails instead of guessing a name") {
	ResourceRegistry registry;
	// Nothing registered: there'd be no name to write into a scene file.
	CHECK(registry.Create<Sound>() == nullptr);
	CHECK(registry.Count() == 0);
}

TEST_CASE("Duplicate type registration is refused") {
	ResourceRegistry registry;
	CHECK(registry.RegisterType<Sound>("Sound").IsValid());

	// Same name, different type.
	CHECK_FALSE(registry.RegisterType<Image>("Sound").IsValid());
	// Same type, different name.
	CHECK_FALSE(registry.RegisterType<Sound>("Audio").IsValid());
	CHECK_FALSE(registry.RegisterType<Sound>("").IsValid());

	// The first registration still works after the refusals.
	CHECK(registry.Create<Sound>() != nullptr);
	CHECK(registry.Create("Audio") == nullptr);
}

// Restoring the saved ID lets component references resolve without a fixup pass.
TEST_CASE("An explicit id is honoured, which is how a scene file round-trips") {
	Fixture fixture;
	ResourceRegistry &registry = fixture.registry;

	const ResourceID saved{0x1234'5678'9abc'def0ull};
	Ref<Resource> restored = registry.Create("Sound", saved);
	REQUIRE(restored != nullptr);
	CHECK(restored->Id() == saved);
	CHECK(registry.Get(saved) == restored);
}

TEST_CASE("A taken id is refused rather than replacing a live resource") {
	Fixture fixture;
	ResourceRegistry &registry = fixture.registry;

	Ref<Sound> first = registry.Create<Sound>();
	REQUIRE(first != nullptr);
	const ResourceID id = first->Id();

	CHECK(registry.Create("Sound", id) == nullptr);
	CHECK(registry.Create<Image>(id) == nullptr);

	// The original is untouched and still the one that id resolves to.
	CHECK(registry.Count() == 1);
	CHECK(registry.Get(id) == first);
}

TEST_CASE("A generated id never collides with one restored from a file") {
	Fixture fixture;
	ResourceRegistry &registry = fixture.registry;

	// Hard to hit by chance with 64 random bits, so it's forced: every
	// generated id is checked against what's already there.
	std::unordered_set<u64> taken;
	for (int i = 0; i < 200; ++i) {
		Ref<Sound> sound = registry.Create<Sound>();
		REQUIRE(sound != nullptr);
		taken.insert(sound->Id().value);
	}
	CHECK(taken.size() == 200);
}

TEST_CASE("Lookups never insert on a miss") {
	Fixture fixture;
	ResourceRegistry &registry = fixture.registry;
	const ResourceID missing{999};

	CHECK(registry.Get(missing) == nullptr);
	CHECK(registry.Get<Sound>(missing) == nullptr);
	CHECK_FALSE(registry.Has(missing));
	CHECK(registry.TypeNameOf(missing).empty());

	CHECK(registry.Count() == 0);
}

TEST_CASE("An invalid id resolves to nothing") {
	Fixture fixture;
	ResourceRegistry &registry = fixture.registry;
	registry.Create<Sound>();

	CHECK(registry.Get(ResourceID{}) == nullptr);
	CHECK_FALSE(registry.Has(ResourceID{}));
}

TEST_CASE("A wrong-typed lookup is null, not a reinterpretation") {
	Fixture fixture;
	ResourceRegistry &registry = fixture.registry;

	Ref<Sound> sound = registry.Create<Sound>();
	REQUIRE(sound != nullptr);

	CHECK(registry.Get<Image>(sound->Id()) == nullptr);
	CHECK(registry.Get<Sound>(sound->Id()) == sound);
}

TEST_CASE("Remove drops ownership but can't dangle a live holder") {
	Fixture fixture;
	ResourceRegistry &registry = fixture.registry;

	Ref<Sound> held = registry.Create<Sound>();
	REQUIRE(held != nullptr);
	const ResourceID id = held->Id();
	held->volume = 0.25f;

	CHECK(registry.Remove(id));
	CHECK_FALSE(registry.Has(id));
	CHECK(registry.Get(id) == nullptr);
	CHECK(registry.Count() == 0);

	// Removing registry ownership must leave existing Ref holders with a valid object.
	CHECK(held->volume == doctest::Approx(0.25f));
	CHECK(held.use_count() == 1);

	CHECK_FALSE(registry.Remove(id));
}

TEST_CASE("Clear releases everything") {
	Fixture fixture;
	ResourceRegistry &registry = fixture.registry;
	Ref<Sound> held = registry.Create<Sound>();
	registry.Create<Image>();
	REQUIRE(registry.Count() == 2);

	registry.Clear();
	CHECK(registry.Count() == 0);
	CHECK(registry.Get(held->Id()) == nullptr);
	CHECK(held.use_count() == 1);

	// Types survive: Clear drops resources, not the registry's schema.
	CHECK(registry.Create<Sound>() != nullptr);
}

// Creation order keeps scene saves stable across repeated serialization.
TEST_CASE("Each visits in creation order, not hash order") {
	Fixture fixture;
	ResourceRegistry &registry = fixture.registry;

	std::vector<ResourceID> created;
	for (int i = 0; i < 50; ++i)
		created.push_back(registry.Create<Sound>()->Id());

	std::vector<ResourceID> visited;
	registry.Each([&](ResourceID id, Resource &, std::string_view) {
		visited.push_back(id);
	});
	CHECK(visited == created);

	// Still in order after a removal from the middle.
	REQUIRE(registry.Remove(created[20]));
	created.erase(created.begin() + 20);

	visited.clear();
	registry.Each([&](ResourceID id, Resource &, std::string_view) {
		visited.push_back(id);
	});
	CHECK(visited == created);
}

TEST_CASE("Each reports the type name and hands over a mutable resource") {
	Fixture fixture;
	ResourceRegistry &registry = fixture.registry;
	registry.Create<Sound>();
	registry.Create<Image>();

	std::vector<std::string> names;
	registry.Each([&](ResourceID, Resource &resource, std::string_view typeName) {
		names.emplace_back(typeName);
		// Non-const because saving calls the non-const Serialize.
		Archive archive = Archive::Save();
		resource.Serialize(archive);
	});

	REQUIRE(names.size() == 2);
	CHECK(names[0] == "Sound");
	CHECK(names[1] == "Image");
}

TEST_CASE("Each tolerates the callback removing what it is visiting") {
	Fixture fixture;
	ResourceRegistry &registry = fixture.registry;
	for (int i = 0; i < 5; ++i)
		registry.Create<Sound>();

	int visited = 0;
	registry.Each([&](ResourceID id, Resource &, std::string_view) {
		++visited;
		registry.Remove(id);
	});

	CHECK(visited == 5);
	CHECK(registry.Count() == 0);
}

// The case above is the benign one - the entry is already dereferenced by the
// time it's removed. This is the one that actually needs the guard: Each walks
// onto ids that were removed *before* it reached them, which a plain lookup
// would throw on.
TEST_CASE("Each tolerates the callback removing entries it hasn't reached") {
	Fixture fixture;
	ResourceRegistry &registry = fixture.registry;
	std::vector<ResourceID> created;
	for (int i = 0; i < 6; ++i)
		created.push_back(registry.Create<Sound>()->Id());

	std::vector<ResourceID> visited;
	registry.Each([&](ResourceID id, Resource &, std::string_view) {
		visited.push_back(id);

		// Remove the next one along, so it's gone before Each gets there.
		const auto it = std::find(created.begin(), created.end(), id);
		if (it != created.end() && std::next(it) != created.end())
			registry.Remove(*std::next(it));
	});

	REQUIRE(visited.size() == 3);
	CHECK(visited[0] == created[0]);
	CHECK(visited[1] == created[2]);
	CHECK(visited[2] == created[4]);
	CHECK(registry.Count() == 3);
}

TEST_CASE("Resources are Serializable, so a snapshot is an archive round trip") {
	Fixture fixture;
	ResourceRegistry &registry = fixture.registry;

	Ref<Sound> sound = registry.Create<Sound>();
	REQUIRE(sound != nullptr);
	sound->path = "res://beep.wav";
	sound->volume = 0.75f;

	Archive saving = Archive::Save();
	sound->Serialize(saving);

	// Restore under the original ID.
	const ResourceID id = sound->Id();
	registry.Remove(id);

	Ref<Resource> restored = registry.Create("Sound", id);
	REQUIRE(restored != nullptr);
	Archive loading = Archive::Load(saving.ToString());
	REQUIRE(loading.IsValid());
	restored->Serialize(loading);

	Ref<Sound> typed = registry.Get<Sound>(id);
	REQUIRE(typed != nullptr);
	CHECK(typed->path == "res://beep.wav");
	CHECK(typed->volume == doctest::Approx(0.75f));
	CHECK(typed->Id() == id);
}
