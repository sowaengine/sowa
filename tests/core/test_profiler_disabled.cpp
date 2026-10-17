// Builds the SOWA_PROFILE_* macros as a build with SOWA_ENABLE_PROFILER=OFF
// sees them. Must stay the first include so profiler.hpp reads this value.
#undef SOWA_PROFILER
#define SOWA_PROFILER 0
#include "core/profiler.hpp"

#include <doctest/doctest.h>

using namespace sowa;

TEST_CASE("With SOWA_PROFILER off the macros record and evaluate nothing") {
	Profiler &profiler = Profiler::Main();
	profiler.Clear();
	profiler.SetEnabled(true);
	profiler.SetFrozen(false);

	int evaluated = 0;
	profiler.BeginFrame();
	REQUIRE(profiler.IsRecording());
	{
		SOWA_PROFILE_PHASES();
		SOWA_PROFILE_PHASE("disabled phase");
		SOWA_PROFILE_SCOPE("disabled scope");
		SOWA_PROFILE_COUNTER("disabled counter", ++evaluated);
	}
	profiler.EndFrame();

	REQUIRE(profiler.FrameCount() == 1);
	CHECK(profiler.FrameAt(0).scopes.empty());
	CHECK(profiler.FrameAt(0).counters.empty());
	CHECK(evaluated == 0);
	profiler.Clear();
}
