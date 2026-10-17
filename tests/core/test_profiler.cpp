#include <doctest/doctest.h>

#include <memory>
#include <thread>

#include "core/profiler.hpp"

using namespace sowa;

namespace {

// Clock that advances 10 ns per reading, so every timestamp is distinct.
void UseStepClock(Profiler &profiler) {
	auto time = std::make_shared<u64>(0);
	profiler.SetClock([time] {
		*time += 10;
		return *time;
	});
}

void RecordFrame(Profiler &profiler, std::initializer_list<ProfileName> scopes) {
	profiler.BeginFrame();
	for (const ProfileName name : scopes) {
		const ProfileScopeGuard scope(profiler, name);
	}
	profiler.EndFrame();
}

} // namespace

TEST_CASE("Profiler records nested scopes in begin order with their depth") {
	Profiler profiler;
	UseStepClock(profiler);
	const ProfileName frame = profiler.Intern("frame");
	const ProfileName update = profiler.Intern("update");
	const ProfileName physics = profiler.Intern("physics");
	const ProfileName draw = profiler.Intern("draw");

	profiler.BeginFrame();
	{
		const ProfileScopeGuard a(profiler, frame);
		{
			const ProfileScopeGuard b(profiler, update);
			const ProfileScopeGuard c(profiler, physics);
		}
		const ProfileScopeGuard d(profiler, draw);
	}
	profiler.EndFrame();

	REQUIRE(profiler.FrameCount() == 1);
	const ProfileFrame &recorded = profiler.FrameAt(0);
	REQUIRE(recorded.scopes.size() == 4);
	CHECK(recorded.scopes[0].name == frame);
	CHECK(recorded.scopes[0].depth == 0);
	CHECK(recorded.scopes[1].name == update);
	CHECK(recorded.scopes[1].depth == 1);
	CHECK(recorded.scopes[2].name == physics);
	CHECK(recorded.scopes[2].depth == 2);
	CHECK(recorded.scopes[3].name == draw);
	CHECK(recorded.scopes[3].depth == 1);

	// Children lie inside their parent, which lies inside the frame.
	CHECK(recorded.beginNs < recorded.scopes[0].beginNs);
	CHECK(recorded.scopes[0].beginNs < recorded.scopes[1].beginNs);
	CHECK(recorded.scopes[2].endNs < recorded.scopes[1].endNs);
	CHECK(recorded.scopes[1].endNs < recorded.scopes[3].beginNs);
	CHECK(recorded.scopes[3].endNs < recorded.scopes[0].endNs);
	CHECK(recorded.scopes[0].endNs < recorded.endNs);
}

TEST_CASE("Profiler interns each name once") {
	Profiler profiler;
	const ProfileName a = profiler.Intern("Player._update");
	const ProfileName b = profiler.Intern("Enemy._update");
	CHECK(a != b);
	CHECK(profiler.Intern(std::string("Player._update")) == a);
	CHECK(profiler.NameOf(a) == "Player._update");
	CHECK(profiler.NameOf(b) == "Enemy._update");
	CHECK(profiler.NameOf(999).empty());
}

TEST_CASE("A scope that disappears and returns keeps its history") {
	Profiler profiler;
	const ProfileName first = profiler.Intern("Spawner._update");
	RecordFrame(profiler, {first});
	RecordFrame(profiler, {});
	// Interned again by text, as a reloaded script's hook would be.
	RecordFrame(profiler, {profiler.Intern("Spawner._update")});

	REQUIRE(profiler.FrameCount() == 3);
	REQUIRE(profiler.FrameAt(0).scopes.size() == 1);
	CHECK(profiler.FrameAt(1).scopes.empty());
	REQUIRE(profiler.FrameAt(2).scopes.size() == 1);
	CHECK(profiler.FrameAt(0).scopes[0].name == profiler.FrameAt(2).scopes[0].name);
}

TEST_CASE("Profiler keeps the newest frames up to its history size") {
	Profiler profiler(3);
	CHECK(profiler.HistorySize() == 3);
	for (int i = 0; i < 5; ++i) {
		RecordFrame(profiler, {});
	}
	REQUIRE(profiler.FrameCount() == 3);
	CHECK(profiler.FrameAt(0).index == 3);
	CHECK(profiler.FrameAt(1).index == 4);
	CHECK(profiler.FrameAt(2).index == 5);

	profiler.Clear();
	CHECK(profiler.FrameCount() == 0);
	RecordFrame(profiler, {});
	CHECK(profiler.FrameCount() == 1);
}

TEST_CASE("A disabled profiler records nothing") {
	Profiler profiler;
	const ProfileName name = profiler.Intern("work");
	profiler.SetEnabled(false);

	profiler.BeginFrame();
	CHECK_FALSE(profiler.IsRecording());
	const Profiler::ScopeToken token = profiler.BeginScope(name);
	CHECK_FALSE(token.valid);
	profiler.SetCounter(name, 1.0);
	profiler.EndFrame();
	CHECK(profiler.FrameCount() == 0);

	profiler.SetEnabled(true);
	RecordFrame(profiler, {name});
	CHECK(profiler.FrameCount() == 1);
}

TEST_CASE("Disabling mid-frame discards that frame") {
	Profiler profiler;
	const ProfileName name = profiler.Intern("work");
	profiler.BeginFrame();
	{
		const ProfileScopeGuard scope(profiler, name);
		profiler.SetEnabled(false);
		profiler.SetEnabled(true);
	}
	profiler.EndFrame();
	CHECK(profiler.FrameCount() == 0);

	RecordFrame(profiler, {name});
	CHECK(profiler.FrameCount() == 1);
}

TEST_CASE("Freezing holds the history until unfrozen") {
	Profiler profiler;
	const ProfileName before = profiler.Intern("before");
	const ProfileName during = profiler.Intern("during");
	RecordFrame(profiler, {before});

	profiler.SetFrozen(true);
	RecordFrame(profiler, {during});
	RecordFrame(profiler, {during});
	REQUIRE(profiler.FrameCount() == 1);
	REQUIRE(profiler.FrameAt(0).scopes.size() == 1);
	CHECK(profiler.FrameAt(0).scopes[0].name == before);

	profiler.SetFrozen(false);
	RecordFrame(profiler, {during});
	REQUIRE(profiler.FrameCount() == 2);
	CHECK(profiler.FrameAt(1).scopes[0].name == during);
}

TEST_CASE("Counters keep the last value set in a frame") {
	Profiler profiler;
	const ProfileName draws = profiler.Intern("draw calls");
	const ProfileName quads = profiler.Intern("quads");

	profiler.BeginFrame();
	profiler.SetCounter(draws, 4.0);
	profiler.SetCounter(quads, 100.0);
	profiler.SetCounter(draws, 7.0);
	profiler.EndFrame();
	RecordFrame(profiler, {});

	const ProfileFrame &first = profiler.FrameAt(0);
	REQUIRE(first.counters.size() == 2);
	CHECK(first.counters[0].name == draws);
	CHECK(first.counters[0].value == 7.0);
	CHECK(first.counters[1].name == quads);
	CHECK(first.counters[1].value == 100.0);
	CHECK(profiler.FrameAt(1).counters.empty());
}

TEST_CASE("A scope still open at EndFrame ends with the frame") {
	Profiler profiler;
	UseStepClock(profiler);
	const ProfileName outer = profiler.Intern("outer");
	const ProfileName inner = profiler.Intern("inner");
	const ProfileName third = profiler.Intern("third");

	profiler.BeginFrame();
	const Profiler::ScopeToken stale = profiler.BeginScope(outer);
	profiler.EndFrame();

	profiler.BeginFrame();
	const Profiler::ScopeToken open = profiler.BeginScope(inner);
	// outer ends in a later frame: its stale token must not end inner.
	profiler.EndScope(stale);
	profiler.EndScope(profiler.BeginScope(third));
	profiler.EndScope(open);
	profiler.EndFrame();

	REQUIRE(profiler.FrameCount() == 2);
	const ProfileFrame &first = profiler.FrameAt(0);
	REQUIRE(first.scopes.size() == 1);
	CHECK(first.scopes[0].endNs == first.endNs);

	const ProfileFrame &second = profiler.FrameAt(1);
	REQUIRE(second.scopes.size() == 2);
	CHECK(second.scopes[0].name == inner);
	CHECK(second.scopes[1].name == third);
	CHECK(second.scopes[1].depth == 1);
	CHECK(second.scopes[1].endNs < second.scopes[0].endNs);
	CHECK(second.scopes[0].endNs < second.endNs);
}

TEST_CASE("Scopes from another thread are ignored") {
	Profiler profiler;
	const ProfileName name = profiler.Intern("worker");
	profiler.BeginFrame();
	std::thread worker([&] {
		const ProfileScopeGuard scope(profiler, name);
		profiler.SetCounter(name, 1.0);
	});
	worker.join();
	profiler.EndFrame();

	REQUIRE(profiler.FrameCount() == 1);
	CHECK(profiler.FrameAt(0).scopes.empty());
	CHECK(profiler.FrameAt(0).counters.empty());
}

TEST_CASE("ProfilePhases times consecutive phases as siblings") {
	Profiler profiler;
	UseStepClock(profiler);
	const ProfileName input = profiler.Intern("input");
	const ProfileName nested = profiler.Intern("nested");
	const ProfileName draw = profiler.Intern("draw");

	profiler.BeginFrame();
	{
		ProfilePhases phases(profiler);
		phases.Next(input);
		{
			const ProfileScopeGuard scope(profiler, nested);
		}
		phases.Next(draw);
	}
	profiler.EndFrame();

	const ProfileFrame &frame = profiler.FrameAt(0);
	REQUIRE(frame.scopes.size() == 3);
	CHECK(frame.scopes[0].name == input);
	CHECK(frame.scopes[0].depth == 0);
	CHECK(frame.scopes[1].name == nested);
	CHECK(frame.scopes[1].depth == 1);
	CHECK(frame.scopes[2].name == draw);
	CHECK(frame.scopes[2].depth == 0);
	CHECK(frame.scopes[0].endNs <= frame.scopes[2].beginNs);
	CHECK(frame.scopes[2].endNs < frame.endNs);
}

TEST_CASE("SOWA_PROFILE macros record into the main profiler") {
	Profiler &profiler = Profiler::Main();
	profiler.Clear();
	profiler.SetEnabled(true);
	profiler.SetFrozen(false);

	int evaluated = 0;
	profiler.BeginFrame();
	{
		SOWA_PROFILE_PHASES();
		SOWA_PROFILE_PHASE("macro phase");
		SOWA_PROFILE_SCOPE("macro scope");
		SOWA_PROFILE_COUNTER("macro counter", ++evaluated);
	}
	profiler.EndFrame();

	REQUIRE(profiler.FrameCount() == 1);
	const ProfileFrame &frame = profiler.FrameAt(0);
	REQUIRE(frame.scopes.size() == 2);
	CHECK(profiler.NameOf(frame.scopes[0].name) == "macro phase");
	CHECK(profiler.NameOf(frame.scopes[1].name) == "macro scope");
	REQUIRE(frame.counters.size() == 1);
	CHECK(profiler.NameOf(frame.counters[0].name) == "macro counter");
	CHECK(frame.counters[0].value == 1.0);

	// Outside a frame the counter value is not evaluated.
	SOWA_PROFILE_COUNTER("macro counter", ++evaluated);
	CHECK(evaluated == 1);
	profiler.Clear();
}
