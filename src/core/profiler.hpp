#pragma once

#include <cstddef>
#include <deque>
#include <functional>
#include <map>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include "core/types.hpp"

// CMake sets SOWA_PROFILER from SOWA_ENABLE_PROFILER. When disabled, profiling
// macros expand to nothing; the Profiler class remains available.
#ifndef SOWA_PROFILER
#define SOWA_PROFILER 0
#endif

namespace sowa {

// Interned scope/counter name. Ids are stable for the process lifetime, so a
// scope that stops reporting and later reports again keeps one history.
using ProfileName = u32;

struct ProfileScopeRecord {
	ProfileName name = 0;
	// Number of enclosing scopes; 0 for a top-level scope. Records are in
	// begin order, so depth alone rebuilds the tree.
	u32 depth = 0;
	u64 beginNs = 0;
	u64 endNs = 0;
};

struct ProfileCounterRecord {
	ProfileName name = 0;
	f64 value = 0.0;
};

struct ProfileFrame {
	// Serial of the recorded frame; gaps mean frames were not recorded
	// (profiler disabled or frozen).
	u64 index = 0;
	u64 beginNs = 0;
	u64 endNs = 0;
	std::vector<ProfileScopeRecord> scopes;
	std::vector<ProfileCounterRecord> counters;
};

// Records scopes and counters on the BeginFrame thread while enabled and unfrozen.
// Disabling or freezing mid-frame discards it; freezing preserves completed history.
class Profiler {
  public:
	static constexpr std::size_t DefaultHistory = 600;

	struct ScopeToken {
		u64 frame = 0;
		u32 index = 0;
		bool valid = false;
	};

	explicit Profiler(std::size_t history = DefaultHistory);

	// The instance the SOWA_PROFILE_* macros record into.
	static Profiler &Main();

	ProfileName Intern(std::string_view name);
	// Interned names are never moved or removed, so the view stays valid.
	std::string_view NameOf(ProfileName name) const;

	void SetEnabled(bool enabled);

	bool IsEnabled() const {
		return _enabled;
	}

	void SetFrozen(bool frozen);

	bool IsFrozen() const {
		return _frozen;
	}

	// Drops the recorded history (and the frame in progress). Names stay.
	void Clear();

	// Discards the current frame and resumes recording at the next BeginFrame.
	// Use for frames held at a debugger breakpoint.
	void DiscardFrame();

	void BeginFrame();
	void EndFrame();

	bool IsRecording() const {
		return _recording;
	}

	ScopeToken BeginScope(ProfileName name) {
		if (!_recording) {
			return {};
		}
		return beginScope(name);
	}

	void EndScope(ScopeToken token);

	// Sets this frame's value for name; a later call in the same frame
	// replaces the earlier value.
	void SetCounter(ProfileName name, f64 value) {
		if (_recording) {
			setCounter(name, value);
		}
	}

	std::size_t HistorySize() const {
		return _ring.size();
	}

	// Completed frames held, at most HistorySize().
	std::size_t FrameCount() const {
		return _count;
	}

	// 0 is the oldest held frame, FrameCount() - 1 the newest.
	const ProfileFrame &FrameAt(std::size_t index) const;

	// Overrides the nanosecond clock (tests). Null restores steady_clock.
	void SetClock(std::function<u64()> clock);

  private:
	ScopeToken beginScope(ProfileName name);
	void setCounter(ProfileName name, f64 value);
	void discardFrame();
	void updateRecording();
	u64 now() const;

	std::deque<std::string> _names;
	std::map<std::string, ProfileName, std::less<>> _ids;

	std::vector<ProfileFrame> _ring;
	std::size_t _head = 0; // next slot to write
	std::size_t _count = 0;

	ProfileFrame _current;
	std::vector<u32> _open; // indices into _current.scopes
	u64 _frameSerial = 0;
	bool _inFrame = false;
	std::thread::id _thread;

	bool _enabled = true;
	bool _frozen = false;
	bool _recording = false;
	std::function<u64()> _clock;
};

// Ends its scope on destruction. Prefer SOWA_PROFILE_SCOPE.
class ProfileScopeGuard {
  public:
	ProfileScopeGuard(Profiler &profiler, ProfileName name)
		: _profiler(profiler), _token(profiler.BeginScope(name)) {
	}

	~ProfileScopeGuard() {
		if (_token.valid) {
			_profiler.EndScope(_token);
		}
	}

	ProfileScopeGuard(const ProfileScopeGuard &) = delete;
	ProfileScopeGuard &operator=(const ProfileScopeGuard &) = delete;

  private:
	Profiler &_profiler;
	Profiler::ScopeToken _token;
};

// Times consecutive phases of one block: Next ends the current phase and
// begins the next one, so a long function is split without nesting it in
// braces. Prefer SOWA_PROFILE_PHASES / SOWA_PROFILE_PHASE.
class ProfilePhases {
  public:
	explicit ProfilePhases(Profiler &profiler) : _profiler(profiler) {
	}

	~ProfilePhases() {
		End();
	}

	ProfilePhases(const ProfilePhases &) = delete;
	ProfilePhases &operator=(const ProfilePhases &) = delete;

	void Next(ProfileName name) {
		End();
		_token = _profiler.BeginScope(name);
	}

	void End() {
		if (_token.valid) {
			_profiler.EndScope(_token);
			_token = {};
		}
	}

  private:
	Profiler &_profiler;
	Profiler::ScopeToken _token;
};

} // namespace sowa

#define SOWA_PROFILE_CONCAT_INNER(a, b) a##b
#define SOWA_PROFILE_CONCAT(a, b) SOWA_PROFILE_CONCAT_INNER(a, b)

#if SOWA_PROFILER
// Times the rest of the enclosing block. name is interned once per call site.
#define SOWA_PROFILE_SCOPE(name)                                                                   \
	static const ::sowa::ProfileName SOWA_PROFILE_CONCAT(sowaProfileName_, __LINE__) =             \
		::sowa::Profiler::Main().Intern(name);                                                     \
	const ::sowa::ProfileScopeGuard SOWA_PROFILE_CONCAT(sowaProfileScope_,                         \
		__LINE__)(::sowa::Profiler::Main(), SOWA_PROFILE_CONCAT(sowaProfileName_, __LINE__))
// Sets this frame's counter value. value is not evaluated unless recording.
#define SOWA_PROFILE_COUNTER(name, value)                                                          \
	do {                                                                                           \
		static const ::sowa::ProfileName sowaProfileCounterName =                                  \
			::sowa::Profiler::Main().Intern(name);                                                 \
		if (::sowa::Profiler::Main().IsRecording()) {                                              \
			::sowa::Profiler::Main().SetCounter(sowaProfileCounterName,                            \
				static_cast<double>(value));                                                       \
		}                                                                                          \
	} while (false)
// Declares the phase timer used by SOWA_PROFILE_PHASE in this block.
#define SOWA_PROFILE_PHASES() ::sowa::ProfilePhases sowaProfilePhases(::sowa::Profiler::Main())
// Ends the current phase of the enclosing SOWA_PROFILE_PHASES and begins name.
#define SOWA_PROFILE_PHASE(name)                                                                   \
	do {                                                                                           \
		static const ::sowa::ProfileName sowaProfilePhaseName =                                    \
			::sowa::Profiler::Main().Intern(name);                                                 \
		sowaProfilePhases.Next(sowaProfilePhaseName);                                              \
	} while (false)
#else
#define SOWA_PROFILE_SCOPE(name) static_cast<void>(0)
#define SOWA_PROFILE_PHASES() static_cast<void>(0)
#define SOWA_PROFILE_PHASE(name) static_cast<void>(0)
#define SOWA_PROFILE_COUNTER(name, value) static_cast<void>(0)
#endif
