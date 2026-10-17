#include <doctest/doctest.h>

#include "core/filesystem/file_system.hpp"
#include "ecs/world.hpp"
#include "editor/play_controls.hpp"
#include "resource/resource_registry.hpp"

using namespace sowa;

TEST_CASE("play controls reflect stopped session prerequisites") {
	auto controls =
		editor::AvailablePlayControls(PlayKind::Stopped, false, true, true);
	CHECK(controls.playProject);
	CHECK(controls.playCurrent);
	CHECK_FALSE(controls.stop);

	controls = editor::AvailablePlayControls(PlayKind::Stopped, false, false, true);
	CHECK(controls.playProject);
	CHECK_FALSE(controls.playCurrent);

	controls = editor::AvailablePlayControls(PlayKind::Stopped, false, true, false);
	CHECK_FALSE(controls.playProject);
	CHECK(controls.playCurrent);
}

TEST_CASE("play controls expose only Stop and Pause during either play mode") {
	for (PlayKind mode : {PlayKind::Project, PlayKind::Current}) {
		const auto controls =
			editor::AvailablePlayControls(mode, false, true, true);
		CHECK_FALSE(controls.playProject);
		CHECK_FALSE(controls.playCurrent);
		CHECK(controls.stop);
		CHECK(controls.pause);
		CHECK_FALSE(controls.resume);
		CHECK_FALSE(controls.step);
	}
}

TEST_CASE("pause controls need a session, and Resume/Step need it paused") {
	auto controls = editor::AvailablePlayControls(PlayKind::Stopped, false, true, true);
	CHECK_FALSE(controls.pause);
	CHECK_FALSE(controls.resume);
	CHECK_FALSE(controls.step);

	controls = editor::AvailablePlayControls(PlayKind::Current, true, true, true);
	CHECK_FALSE(controls.pause);
	CHECK(controls.resume);
	CHECK(controls.step);
	CHECK(controls.stop);
	CHECK_FALSE(controls.playProject);
	CHECK_FALSE(controls.playCurrent);
}

TEST_CASE("RequestPlayControl queues the same current-play and stop transitions") {
	World world;
	ResourceRegistry resources;
	FileSystem fs;
	SceneManager scenes(world, resources, fs);
	Scene level = scenes.Create("Level");
	REQUIRE(scenes.SetCurrent(level));

	CHECK(editor::RequestPlayControl(
		scenes, editor::PlayControl::PlayCurrent, ""));
	CHECK_FALSE(scenes.IsPlaying());
	scenes.Flush();
	CHECK(scenes.PlayMode() == PlayKind::Current);
	CHECK_FALSE(editor::RequestPlayControl(
		scenes, editor::PlayControl::PlayProject, "res://main.scene"));

	CHECK(editor::RequestPlayControl(scenes, editor::PlayControl::Stop, ""));
	CHECK(scenes.IsPlaying());
	scenes.Flush();
	CHECK_FALSE(scenes.IsPlaying());
}

TEST_CASE("RequestPlayControl pauses, steps and resumes a session") {
	World world;
	ResourceRegistry resources;
	FileSystem fs;
	SceneManager scenes(world, resources, fs);
	Scene level = scenes.Create("Level");
	REQUIRE(scenes.SetCurrent(level));

	CHECK_FALSE(editor::RequestPlayControl(scenes, editor::PlayControl::Pause, ""));
	REQUIRE(editor::RequestPlayControl(scenes, editor::PlayControl::PlayCurrent, ""));
	scenes.Flush();

	CHECK_FALSE(editor::RequestPlayControl(scenes, editor::PlayControl::Step, ""));
	CHECK(editor::RequestPlayControl(scenes, editor::PlayControl::Pause, ""));
	CHECK(scenes.IsPaused());
	CHECK(editor::RequestPlayControl(scenes, editor::PlayControl::Step, ""));
	CHECK(scenes.TakeStep());
	CHECK(editor::RequestPlayControl(scenes, editor::PlayControl::Resume, ""));
	CHECK_FALSE(scenes.IsPaused());
}

TEST_CASE("Frame step is unavailable while the debugger holds a frame") {
	const auto paused = editor::AvailablePlayControls(PlayKind::Current, true, true, true);
	CHECK(paused.step);
	const auto suspended = editor::AvailablePlayControls(PlayKind::Current, true, true, true, true);
	CHECK_FALSE(suspended.step);
	CHECK(suspended.resume);
	CHECK(suspended.stop);
}
