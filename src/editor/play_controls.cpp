#include "editor/play_controls.hpp"

namespace sowa::editor {

PlayControlAvailability AvailablePlayControls(PlayKind mode, bool paused, bool hasCurrentScene,
	bool hasMainScene, bool scriptSuspended) {
	const bool stopped = mode == PlayKind::Stopped;
	return PlayControlAvailability{
		stopped && hasMainScene,
		stopped && hasCurrentScene,
		!stopped,
		!stopped && !paused,
		!stopped && paused,
		!stopped && paused && !scriptSuspended,
	};
}

bool RequestPlayControl(SceneManager &scenes, PlayControl control, std::string_view mainScene) {
	const PlayControlAvailability available =
		AvailablePlayControls(scenes.PlayMode(), scenes.IsPaused(), scenes.Current().IsValid(),
			!mainScene.empty(), scenes.IsScriptSuspended());
	switch (control) {
	case PlayControl::PlayProject:
		if (!available.playProject) {
			return false;
		}
		scenes.PlayProject(mainScene);
		return true;
	case PlayControl::PlayCurrent:
		if (!available.playCurrent) {
			return false;
		}
		scenes.PlayCurrent();
		return true;
	case PlayControl::Stop:
		if (!available.stop) {
			return false;
		}
		scenes.Stop();
		return true;
	case PlayControl::Pause:
		if (!available.pause) {
			return false;
		}
		scenes.Pause();
		return true;
	case PlayControl::Resume:
		if (!available.resume) {
			return false;
		}
		scenes.Resume();
		return true;
	case PlayControl::Step:
		if (!available.step) {
			return false;
		}
		scenes.Step();
		return true;
	}
	return false;
}

} // namespace sowa::editor
