#pragma once

#include <functional>
#include <utility>

#include "core/types.hpp"

namespace sowa {

// Caller-driven countdown timer
//
// States: Stopped (elapsed reset), Running, Paused (elapsed preserved).
class Timer {
  public:
	enum class State {
		Stopped,
		Running,
		Paused,
	};

	Timer() = default;

	explicit Timer(f32 timeout, bool autoStart = false, bool repeating = false)
		: _timeout(timeout), _repeating(repeating) {
		if (autoStart) {
			_state = State::Running;
		}
	}

	void Start() {
		if (_state == State::Running) {
			return;
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

	// Returns whether OnTimeout fired. Repeating timers catch up after large deltas;
	// timeout <= 0 fires once per Update. One-shot timers stop after firing.
	bool Update(f32 delta) {
		if (_state != State::Running) {
			return false;
		}

		bool fired = false;

		if (_timeout <= 0.f) {
			fire();
			fired = true;
			if (!_repeating) {
				Stop();
			}
			return fired;
		}

		_elapsed += delta;
		while (_elapsed >= _timeout) {
			_elapsed -= _timeout;
			fire();
			fired = true;
			if (!_repeating) {
				Stop();
				break;
			}
		}
		return fired;
	}

	void SetOnTimeout(std::function<void()> callback) {
		_onTimeout = std::move(callback);
	}

	void ClearOnTimeout() {
		_onTimeout = nullptr;
	}

	bool IsRunning() const {
		return _state == State::Running;
	}

	bool IsPaused() const {
		return _state == State::Paused;
	}

	State GetState() const {
		return _state;
	}

	f32 Elapsed() const {
		return _elapsed;
	}

	f32 Remaining() const {
		if (_timeout <= 0.f) {
			return 0.f;
		}
		const f32 remaining = _timeout - _elapsed;
		return remaining > 0.f ? remaining : 0.f;
	}

	f32 Timeout() const {
		return _timeout;
	}

	void SetTimeout(f32 timeout) {
		_timeout = timeout;
	}

	bool IsRepeating() const {
		return _repeating;
	}

	void SetRepeating(bool repeating) {
		_repeating = repeating;
	}

  private:
	void fire() {
		if (_onTimeout) {
			_onTimeout();
		}
	}

	std::function<void()> _onTimeout;
	f32 _elapsed = 0.f;
	f32 _timeout = 1.f;
	bool _repeating = false;
	State _state = State::Stopped;
};

} // namespace sowa
