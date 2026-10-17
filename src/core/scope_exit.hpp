#pragma once

#include <functional>
#include <type_traits>
#include <utility>

namespace sowa {

// Runs the callback on scope exit unless released.
template <typename Callback> class ScopeExit {
  public:
	static_assert(std::is_invocable_v<Callback &>,
		"ScopeExit callback must be callable without arguments");

	explicit ScopeExit(Callback callback) noexcept(std::is_nothrow_move_constructible_v<Callback>)
		: _callback(std::move(callback)) {
	}

	ScopeExit(const ScopeExit &) = delete;
	ScopeExit &operator=(const ScopeExit &) = delete;
	ScopeExit &operator=(ScopeExit &&) = delete;

	ScopeExit(ScopeExit &&other) noexcept(std::is_nothrow_move_constructible_v<Callback>)
		: _callback(std::move(other._callback)), _active(other._active) {
		other._active = false;
	}

	~ScopeExit() noexcept(std::is_nothrow_invocable_v<Callback &>) {
		if (_active) {
			std::invoke(_callback);
		}
	}

	void Release() noexcept {
		_active = false;
	}

  private:
	Callback _callback;
	bool _active = true;
};

template <typename Callback> [[nodiscard]] auto MakeScopeExit(Callback &&callback) {
	return ScopeExit<std::decay_t<Callback>>(std::forward<Callback>(callback));
}

} // namespace sowa
