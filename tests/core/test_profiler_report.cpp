#include <doctest/doctest.h>

#include <memory>
#include <string>

#include <yaml-cpp/yaml.h>

#include "core/profiler.hpp"
#include "core/profiler_report.hpp"

using namespace sowa;

namespace {

// Clock the test sets explicitly, in nanoseconds.
struct ManualClock {
	std::shared_ptr<u64> now = std::make_shared<u64>(0);

	void Attach(Profiler &profiler) const {
		profiler.SetClock([now = now] { return *now; });
	}

	void Set(u64 ns) const {
		*now = ns;
	}
};

constexpr u64 kMs = 1'000'000;

// frame 1: [0, 10 ms] update [1, 7] (physics [2, 5]), draw [7, 9]
// frame 2: [10, 30 ms] update [11, 15], hook [15, 16], hook [16, 19]
void RecordTwoFrames(Profiler &profiler, const ManualClock &clock) {
	const ProfileName update = profiler.Intern("update");
	const ProfileName physics = profiler.Intern("physics");
	const ProfileName draw = profiler.Intern("draw");
	const ProfileName hook = profiler.Intern("Lua.Enemy._exit");
	const ProfileName draws = profiler.Intern("Draw calls");

	clock.Set(0);
	profiler.BeginFrame();
	clock.Set(1 * kMs);
	auto u = profiler.BeginScope(update);
	clock.Set(2 * kMs);
	auto p = profiler.BeginScope(physics);
	clock.Set(5 * kMs);
	profiler.EndScope(p);
	clock.Set(7 * kMs);
	profiler.EndScope(u);
	auto d = profiler.BeginScope(draw);
	clock.Set(9 * kMs);
	profiler.EndScope(d);
	profiler.SetCounter(draws, 8.0);
	clock.Set(10 * kMs);
	profiler.EndFrame();

	profiler.BeginFrame();
	clock.Set(11 * kMs);
	u = profiler.BeginScope(update);
	clock.Set(15 * kMs);
	profiler.EndScope(u);
	auto h = profiler.BeginScope(hook);
	clock.Set(16 * kMs);
	profiler.EndScope(h);
	h = profiler.BeginScope(hook);
	clock.Set(19 * kMs);
	profiler.EndScope(h);
	profiler.SetCounter(draws, 4.0);
	clock.Set(30 * kMs);
	profiler.EndFrame();
}

} // namespace

TEST_CASE("SummarizeScopes averages per-frame totals over every held frame") {
	Profiler profiler;
	ManualClock clock;
	clock.Attach(profiler);
	RecordTwoFrames(profiler, clock);

	const std::vector<ProfileScopeSummary> summaries = SummarizeScopes(profiler);
	REQUIRE(summaries.size() == 4);
	// Highest average first.
	CHECK(profiler.NameOf(summaries[0].name) == "update");
	CHECK(summaries[0].averageMs == doctest::Approx(5.0)); // (6 + 4) / 2
	CHECK(summaries[0].maxMs == doctest::Approx(6.0));
	CHECK(summaries[0].callsPerFrame == doctest::Approx(1.0));

	// The hook ran twice in frame 2: totals add up, absent frames count as 0.
	CHECK(profiler.NameOf(summaries[1].name) == "Lua.Enemy._exit");
	CHECK(summaries[1].averageMs == doctest::Approx(2.0)); // (0 + 4) / 2
	CHECK(summaries[1].maxMs == doctest::Approx(4.0));
	CHECK(summaries[1].callsPerFrame == doctest::Approx(1.0)); // 2 calls / 2 frames

	CHECK(profiler.NameOf(summaries[2].name) == "physics");
	CHECK(summaries[2].averageMs == doctest::Approx(1.5));
	CHECK(profiler.NameOf(summaries[3].name) == "draw");
	CHECK(summaries[3].averageMs == doctest::Approx(1.0));
}

TEST_CASE("SummarizeCounters averages over the frames that reported") {
	Profiler profiler;
	ManualClock clock;
	clock.Attach(profiler);
	RecordTwoFrames(profiler, clock);
	profiler.BeginFrame(); // a frame without counters does not dilute them
	profiler.EndFrame();

	const std::vector<ProfileCounterSummary> counters = SummarizeCounters(profiler);
	REQUIRE(counters.size() == 1);
	CHECK(profiler.NameOf(counters[0].name) == "Draw calls");
	CHECK(counters[0].average == doctest::Approx(6.0));
	CHECK(counters[0].max == doctest::Approx(8.0));
}

TEST_CASE("Summaries of an empty profiler are empty") {
	Profiler profiler;
	CHECK(SummarizeScopes(profiler).empty());
	CHECK(SummarizeCounters(profiler).empty());
}

TEST_CASE("ScopeSelfTimesMs subtracts direct children only") {
	Profiler profiler;
	ManualClock clock;
	clock.Attach(profiler);
	RecordTwoFrames(profiler, clock);

	const ProfileFrame &frame = profiler.FrameAt(0);
	CHECK(FrameDurationMs(frame) == doctest::Approx(10.0));
	const std::vector<f64> self = ScopeSelfTimesMs(frame);
	REQUIRE(self.size() == 3);
	CHECK(self[0] == doctest::Approx(3.0)); // update 6 - physics 3
	CHECK(self[1] == doctest::Approx(3.0)); // physics, a leaf
	CHECK(self[2] == doctest::Approx(2.0)); // draw: a sibling, not update's child
}

TEST_CASE("ToChromeTrace writes frames, scopes and counters as trace events") {
	Profiler profiler;
	ManualClock clock;
	clock.Attach(profiler);
	clock.Set(5 * kMs); // timestamps are relative to the oldest frame
	profiler.BeginFrame();
	const ProfileName odd = profiler.Intern("quote\" back\\slash");
	clock.Set(6 * kMs);
	const auto token = profiler.BeginScope(odd);
	clock.Set(8 * kMs);
	profiler.EndScope(token);
	profiler.SetCounter(profiler.Intern("Quads"), 12.5);
	clock.Set(9 * kMs);
	profiler.EndFrame();

	// JSON is YAML, so yaml-cpp parses the result back.
	const YAML::Node trace = YAML::Load(ToChromeTrace(profiler));
	CHECK(trace["displayTimeUnit"].as<std::string>() == "ms");
	const YAML::Node events = trace["traceEvents"];
	REQUIRE(events.IsSequence());
	REQUIRE(events.size() == 3);

	CHECK(events[0]["name"].as<std::string>() == "Frame");
	CHECK(events[0]["ph"].as<std::string>() == "X");
	CHECK(events[0]["ts"].as<double>() == doctest::Approx(0.0));
	CHECK(events[0]["dur"].as<double>() == doctest::Approx(4000.0));
	CHECK(events[0]["args"]["index"].as<u64>() == 1);

	CHECK(events[1]["name"].as<std::string>() == "quote\" back\\slash");
	CHECK(events[1]["ph"].as<std::string>() == "X");
	CHECK(events[1]["ts"].as<double>() == doctest::Approx(1000.0));
	CHECK(events[1]["dur"].as<double>() == doctest::Approx(2000.0));
	CHECK(events[1]["tid"].as<int>() == events[0]["tid"].as<int>());

	CHECK(events[2]["name"].as<std::string>() == "Quads");
	CHECK(events[2]["ph"].as<std::string>() == "C");
	CHECK(events[2]["ts"].as<double>() == doctest::Approx(0.0));
	CHECK(events[2]["args"]["value"].as<double>() == doctest::Approx(12.5));
}

TEST_CASE("ToChromeTrace of an empty profiler is an empty event list") {
	Profiler profiler;
	const YAML::Node trace = YAML::Load(ToChromeTrace(profiler));
	REQUIRE(trace["traceEvents"].IsSequence());
	CHECK(trace["traceEvents"].size() == 0);
}
