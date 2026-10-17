#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>

namespace sowa {

// Editor watchdog. Beat signals main-thread progress. While armed, a background
// thread calls onTimeout once after the timeout without a Beat; Application uses
// RequestPause to interrupt Lua without freezing the editor.
class ScriptWatchdog {
  public:
	using Clock = std::chrono::steady_clock;

	ScriptWatchdog(Clock::duration timeout, std::function<void()> onTimeout);
	~ScriptWatchdog();

	ScriptWatchdog(const ScriptWatchdog &) = delete;
	ScriptWatchdog &operator=(const ScriptWatchdog &) = delete;

	// Starts and stops the background thread. Stop joins it.
	void Start();
	void Stop();

	// The main thread is responsive at now. Returns true when the watchdog
	// had fired since the previous Beat, so the caller can cancel a request
	// that never reached a script.
	bool Beat(Clock::time_point now = Clock::now());

	// Only an armed watchdog fires (Application arms it during Play).
	void SetArmed(bool armed);

	// Whether it fired since the last Beat.
	bool HasFired() const {
		return _fired.load();
	}

	Clock::duration Timeout() const {
		return _timeout;
	}

	// One check at now, as the background thread runs it. Public so tests
	// can drive time without the thread.
	void Check(Clock::time_point now);

  private:
	const Clock::duration _timeout;
	std::function<void()> _onTimeout;
	std::atomic<Clock::rep> _lastBeat;
	std::atomic<bool> _armed{false};
	std::atomic<bool> _fired{false};

	std::thread _thread;
	std::mutex _mutex;
	std::condition_variable _wake;
	bool _stopping = false;
};

} // namespace sowa
