#pragma once

#include <chrono>
#include <functional>
#include <utility>

#include "core/types.hpp"

namespace sowa {

// Measures frame delta with a steady clock. Rebase excludes time spent outside the
// frame loop, such as a debugger stop, from the next delta.
class FrameClock {
  public:
	// Seconds from an arbitrary epoch. Defaults to steady_clock.
	using Now = std::function<f64()>;

	explicit FrameClock(Now now = {}) : _now(std::move(now)) {
		_last = read();
	}

	// Seconds since the previous Tick or Rebase.
	f32 Tick() {
		const f64 now = read();
		const f64 delta = now - _last;
		_last = now;
		return static_cast<f32>(delta);
	}

	void Rebase() {
		_last = read();
	}

  private:
	f64 read() const {
		if (_now) {
			return _now();
		}
		return std::chrono::duration<f64>(std::chrono::steady_clock::now().time_since_epoch())
			.count();
	}

	Now _now;
	f64 _last = 0.0;
};

} // namespace sowa
