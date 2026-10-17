#include <doctest/doctest.h>

#include "audio/audio_mixer.hpp"
#include "core/filesystem/file_system.hpp"
#include "ecs/world.hpp"
#include "resource/resource_registry.hpp"
#include "scene/scene_manager.hpp"
#include "script/script_suspension.hpp"

using namespace sowa;

namespace {

struct Fixture {
	World world;
	ResourceRegistry resources;
	FileSystem fs;
	SceneManager scenes{world, resources, fs};
	AudioMixer mixer;
	ScriptSuspension suspension{scenes, mixer};
	LuaStopEvent event;

	Fixture() {
		event.source = "res://player.lua";
		event.line = 12;
	}

	void play() {
		REQUIRE(scenes.SetCurrent(scenes.Create("Level")));
		scenes.PlayCurrent();
		scenes.Flush();
		REQUIRE(scenes.IsPlaying());
	}
};

} // namespace

TEST_CASE("Outside a play session a debugger stop continues at once") {
	Fixture f;
	int pumped = 0;
	const LuaResumeAction action = f.suspension.Run(f.event, [&] {
		++pumped;
		return true;
	});
	CHECK(action == LuaResumeAction::Continue);
	CHECK(pumped == 0);
	CHECK_FALSE(f.scenes.IsPaused());
	CHECK_FALSE(f.suspension.WouldSuspend());
}

TEST_CASE("Without an editor, stops outside Play are held too") {
	Fixture f;
	f.suspension.SetSuspendOutsidePlay(true);
	CHECK(f.suspension.WouldSuspend());
	int pumped = 0;
	const LuaResumeAction action = f.suspension.Run(f.event, [&] {
		if (++pumped == 2) {
			f.suspension.Request(LuaResumeAction::StepIn);
		}
		return true;
	});
	CHECK(action == LuaResumeAction::StepIn);
	CHECK(pumped == 2);
	CHECK_FALSE(f.scenes.IsPlaying());
}

TEST_CASE("A stop pauses the session and silences audio until continued") {
	Fixture f;
	f.play();
	int pumped = 0;
	const LuaResumeAction action = f.suspension.Run(f.event, [&] {
		++pumped;
		CHECK(f.suspension.IsActive());
		CHECK(f.suspension.Event().line == 12);
		CHECK(f.scenes.IsPaused());
		CHECK(f.scenes.IsScriptSuspended());
		CHECK(f.mixer.IsPaused());
		if (pumped == 3) {
			f.suspension.Request(LuaResumeAction::StepOver);
		}
		return true;
	});
	CHECK(pumped == 3);
	CHECK(action == LuaResumeAction::StepOver);
	CHECK_FALSE(f.suspension.IsActive());
	CHECK_FALSE(f.scenes.IsScriptSuspended());
	// The session was running before the stop and runs again.
	CHECK_FALSE(f.scenes.IsPaused());
	CHECK_FALSE(f.mixer.IsPaused());
}

TEST_CASE("Continuing a stop in an already-paused session leaves it paused") {
	Fixture f;
	f.play();
	f.scenes.Pause();
	f.mixer.SetPaused(true);
	const LuaResumeAction action = f.suspension.Run(f.event, [&] {
		f.suspension.Request(LuaResumeAction::Continue);
		return true;
	});
	CHECK(action == LuaResumeAction::Continue);
	CHECK(f.scenes.IsPaused());
	CHECK(f.mixer.IsPaused());
}

TEST_CASE("Resuming the session from the toolbar continues it running") {
	Fixture f;
	f.play();
	f.scenes.Pause();
	const LuaResumeAction action = f.suspension.Run(f.event, [&] {
		f.scenes.Resume();
		return true;
	});
	CHECK(action == LuaResumeAction::Continue);
	CHECK_FALSE(f.scenes.IsPaused());
	CHECK_FALSE(f.mixer.IsPaused());
}

TEST_CASE("Stop while suspended aborts the Lua call before the Stop applies") {
	Fixture f;
	f.play();
	const LuaResumeAction action = f.suspension.Run(f.event, [&] {
		f.scenes.Stop();
		return true;
	});
	CHECK(action == LuaResumeAction::Abort);
	// Still playing: the queued Stop applies at the next Flush, after the
	// aborted call has unwound.
	CHECK(f.scenes.IsPlaying());
	CHECK(f.scenes.IsStopPending());
	CHECK_FALSE(f.scenes.IsScriptSuspended());

	// Further stops in the same frame do not suspend again.
	int pumped = 0;
	CHECK(f.suspension.Run(f.event, [&] {
		++pumped;
		return true;
	}) == LuaResumeAction::Abort);
	CHECK(pumped == 0);

	f.scenes.Flush();
	CHECK_FALSE(f.scenes.IsPlaying());
	CHECK_FALSE(f.scenes.IsStopPending());
	CHECK_FALSE(f.scenes.IsPaused());
}

TEST_CASE("Closing the window while suspended aborts") {
	Fixture f;
	f.play();
	CHECK(f.suspension.Run(f.event, [] { return false; }) == LuaResumeAction::Abort);
	CHECK_FALSE(f.scenes.IsScriptSuspended());
}

TEST_CASE("Request outside a stop is ignored") {
	Fixture f;
	f.play();
	f.suspension.Request(LuaResumeAction::Abort);
	int pumped = 0;
	const LuaResumeAction action = f.suspension.Run(f.event, [&] {
		if (++pumped == 2) {
			f.suspension.Request(LuaResumeAction::Continue);
		}
		return true;
	});
	CHECK(action == LuaResumeAction::Continue);
	CHECK(pumped == 2);
}

TEST_CASE("A request ends only the stop it was made in") {
	Fixture f;
	f.play();
	CHECK(f.suspension.Run(f.event, [&] {
		f.suspension.Request(LuaResumeAction::StepIn);
		f.scenes.Resume();
		return true;
	}) == LuaResumeAction::StepIn);

	int pumped = 0;
	const LuaResumeAction action = f.suspension.Run(f.event, [&] {
		if (++pumped == 3) {
			f.suspension.Request(LuaResumeAction::Continue);
		}
		return true;
	});
	CHECK(pumped == 3);
	CHECK(action == LuaResumeAction::Continue);
}
