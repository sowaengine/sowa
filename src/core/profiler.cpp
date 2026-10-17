#include "core/profiler.hpp"

#include <algorithm>
#include <chrono>
#include <utility>

namespace sowa {

Profiler::Profiler(std::size_t history) : _ring(std::max<std::size_t>(history, 1)) {
	updateRecording();
}

Profiler &Profiler::Main() {
	static Profiler profiler;
	return profiler;
}

ProfileName Profiler::Intern(std::string_view name) {
	if (const auto it = _ids.find(name); it != _ids.end()) {
		return it->second;
	}
	const auto id = static_cast<ProfileName>(_names.size());
	_names.emplace_back(name);
	_ids.emplace(_names.back(), id);
	return id;
}

std::string_view Profiler::NameOf(ProfileName name) const {
	if (name >= _names.size()) {
		return {};
	}
	return _names[name];
}

void Profiler::SetEnabled(bool enabled) {
	if (_enabled == enabled) {
		return;
	}
	_enabled = enabled;
	discardFrame();
}

void Profiler::SetFrozen(bool frozen) {
	if (_frozen == frozen) {
		return;
	}
	_frozen = frozen;
	discardFrame();
}

void Profiler::Clear() {
	for (ProfileFrame &frame : _ring) {
		frame.scopes.clear();
		frame.counters.clear();
	}
	_head = 0;
	_count = 0;
	discardFrame();
}

void Profiler::DiscardFrame() {
	discardFrame();
}

void Profiler::BeginFrame() {
	// An unfinished frame (EndFrame never called) is dropped, not committed.
	_current.scopes.clear();
	_current.counters.clear();
	_open.clear();
	++_frameSerial;
	_current.index = _frameSerial;
	_current.beginNs = now();
	_current.endNs = _current.beginNs;
	_thread = std::this_thread::get_id();
	_inFrame = true;
	updateRecording();
}

void Profiler::EndFrame() {
	if (!_recording) {
		_inFrame = false;
		updateRecording();
		return;
	}
	const u64 end = now();
	// Scopes still open at the frame boundary end with the frame; their guards
	// find a stale token and do nothing.
	for (const u32 index : _open) {
		_current.scopes[index].endNs = end;
	}
	_open.clear();
	_current.endNs = end;

	std::swap(_ring[_head], _current);
	_head = (_head + 1) % _ring.size();
	_count = std::min(_count + 1, _ring.size());

	_current.scopes.clear();
	_current.counters.clear();
	_inFrame = false;
	updateRecording();
}

void Profiler::EndScope(ScopeToken token) {
	// A token from an earlier frame is stale: its scope was closed by EndFrame.
	// Within a frame, guards end in reverse order, so the token is the top.
	if (!_recording || !token.valid || token.frame != _current.index) {
		return;
	}
	_current.scopes[token.index].endNs = now();
	_open.pop_back();
}

const ProfileFrame &Profiler::FrameAt(std::size_t index) const {
	const std::size_t oldest = (_head + _ring.size() - _count) % _ring.size();
	return _ring[(oldest + index) % _ring.size()];
}

void Profiler::SetClock(std::function<u64()> clock) {
	_clock = std::move(clock);
}

Profiler::ScopeToken Profiler::beginScope(ProfileName name) {
	if (std::this_thread::get_id() != _thread) {
		return {};
	}
	const auto index = static_cast<u32>(_current.scopes.size());
	_current.scopes.push_back(ProfileScopeRecord{name, static_cast<u32>(_open.size()), now(), 0});
	_open.push_back(index);
	return ScopeToken{_current.index, index, true};
}

void Profiler::setCounter(ProfileName name, f64 value) {
	if (std::this_thread::get_id() != _thread) {
		return;
	}
	for (ProfileCounterRecord &counter : _current.counters) {
		if (counter.name == name) {
			counter.value = value;
			return;
		}
	}
	_current.counters.push_back(ProfileCounterRecord{name, value});
}

void Profiler::discardFrame() {
	_current.scopes.clear();
	_current.counters.clear();
	_open.clear();
	_inFrame = false;
	updateRecording();
}

void Profiler::updateRecording() {
	_recording = _enabled && !_frozen && _inFrame;
}

u64 Profiler::now() const {
	if (_clock) {
		return _clock();
	}
	return static_cast<u64>(std::chrono::duration_cast<std::chrono::nanoseconds>(
		std::chrono::steady_clock::now().time_since_epoch())
			.count());
}

} // namespace sowa
