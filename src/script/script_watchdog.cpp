#include "script/script_watchdog.hpp"

#include <utility>

namespace sowa {

namespace {

// How often the background thread looks at the last beat.
constexpr std::chrono::milliseconds kPollInterval{50};

} // namespace

ScriptWatchdog::ScriptWatchdog(Clock::duration timeout, std::function<void()> onTimeout)
	: _timeout(timeout), _onTimeout(std::move(onTimeout)),
	  _lastBeat(Clock::now().time_since_epoch().count()) {
}

ScriptWatchdog::~ScriptWatchdog() {
	Stop();
}

void ScriptWatchdog::Start() {
	if (_thread.joinable()) {
		return;
	}
	{
		std::lock_guard lock(_mutex);
		_stopping = false;
	}
	Beat();
	_thread = std::thread([this] {
		std::unique_lock lock(_mutex);
		while (!_wake.wait_for(lock, kPollInterval, [this] { return _stopping; })) {
			lock.unlock();
			Check(Clock::now());
			lock.lock();
		}
	});
}

void ScriptWatchdog::Stop() {
	{
		std::lock_guard lock(_mutex);
		_stopping = true;
	}
	_wake.notify_all();
	if (_thread.joinable()) {
		_thread.join();
	}
}

bool ScriptWatchdog::Beat(Clock::time_point now) {
	_lastBeat = now.time_since_epoch().count();
	return _fired.exchange(false);
}

void ScriptWatchdog::SetArmed(bool armed) {
	_armed = armed;
}

void ScriptWatchdog::Check(Clock::time_point now) {
	if (!_armed.load() || _fired.load()) {
		return;
	}
	const Clock::time_point lastBeat{Clock::duration(_lastBeat.load())};
	if (now - lastBeat >= _timeout) {
		_fired = true;
		if (_onTimeout) {
			_onTimeout();
		}
	}
}

} // namespace sowa
