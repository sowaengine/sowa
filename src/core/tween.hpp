#pragma once

#include <functional>
#include <utility>

#include "core/types.hpp"
#include "math/easing.hpp"

namespace sowa {

// Caller-driven 0..1 tween with linear, named, or custom easing. Pause preserves elapsed
// time; Stop resets it; Finished has already fired on_finish.
class Tween {
  public:
	enum class State {
		Stopped,
		Running,
		Paused,
		Finished,
	};

	Tween() = default;

	explicit Tween(f32 duration, bool autoStart = true) : _duration(duration) {
		if (autoStart) {
			_state = State::Running;
		}
	}

	void Start() {
		if (_state == State::Running) {
			return;
		}
		if (_state == State::Finished || _state == State::Stopped) {
			_elapsed = 0.f;
		}
		_state = State::Running;
	}

	void Pause() {
		if (_state != State::Running) {
			return;
		}
		_state = State::Paused;
	}

	void Stop() {
		_state = State::Stopped;
		_elapsed = 0.f;
	}

	// Advances the tween. Returns true if on_finish fired during this call.
	// A large delta jumps to the end (callback 1, then finish) rather than
	// sampling in-between values. duration <= 0 fires 1 and finishes once.
	bool Update(f32 delta) {
		if (_state != State::Running) {
			return false;
		}

		if (_duration <= 0.f) {
			fireUpdate(eased(1.f));
			if (_state != State::Running) {
				return false;
			}
			finish();
			return true;
		}

		_elapsed += delta;
		const f32 t = progress();
		fireUpdate(eased(t));
		if (_state != State::Running) {
			return false;
		}
		if (t < 1.f) {
			return false;
		}
		finish();
		return true;
	}

	void SetOnUpdate(std::function<void(f32)> callback) {
		_onUpdate = std::move(callback);
	}

	void ClearOnUpdate() {
		_onUpdate = nullptr;
	}

	void SetOnFinish(std::function<void()> callback) {
		_onFinish = std::move(callback);
	}

	void ClearOnFinish() {
		_onFinish = nullptr;
	}

	// Identity when empty. SetEase(Easing) stores Math::Ease for that curve.
	void SetEase(std::function<f32(f32)> ease) {
		_ease = std::move(ease);
	}

	void SetEase(Easing easing) {
		if (easing == Easing::Linear) {
			_ease = nullptr;
			return;
		}
		_ease = [easing](f32 x) { return Math::Ease(x, easing); };
	}

	void ClearEase() {
		_ease = nullptr;
	}

	bool IsRunning() const {
		return _state == State::Running;
	}

	bool IsPaused() const {
		return _state == State::Paused;
	}

	bool IsFinished() const {
		return _state == State::Finished;
	}

	State GetState() const {
		return _state;
	}

	f32 Elapsed() const {
		return _elapsed;
	}

	f32 Remaining() const {
		if (_duration <= 0.f) {
			return 0.f;
		}
		const f32 remaining = _duration - _elapsed;
		return remaining > 0.f ? remaining : 0.f;
	}

	f32 Duration() const {
		return _duration;
	}

	void SetDuration(f32 duration) {
		_duration = duration;
	}

	f32 Progress() const {
		return progress();
	}

  private:
	f32 progress() const {
		if (_duration <= 0.f) {
			return (_state == State::Finished) ? 1.f : 0.f;
		}
		f32 t = _elapsed / _duration;
		if (t < 0.f) {
			return 0.f;
		}
		if (t > 1.f) {
			return 1.f;
		}
		return t;
	}

	f32 eased(f32 t) const {
		return _ease ? _ease(t) : t;
	}

	void fireUpdate(f32 value) {
		if (_onUpdate) {
			_onUpdate(value);
		}
	}

	void finish() {
		_state = State::Finished;
		if (_duration > 0.f) {
			_elapsed = _duration;
		}
		if (_onFinish) {
			_onFinish();
		}
	}

	std::function<void(f32)> _onUpdate;
	std::function<void()> _onFinish;
	std::function<f32(f32)> _ease;
	f32 _elapsed = 0.f;
	f32 _duration = 1.f;
	State _state = State::Stopped;
};

} // namespace sowa
