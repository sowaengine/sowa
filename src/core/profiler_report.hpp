#pragma once

#include <string>
#include <vector>

#include "core/profiler.hpp"
#include "core/types.hpp"

namespace sowa {

// Views derived on demand from profiler history, shared by the editor and trace export.

// Timing of one scope name over the held frames. A frame's value for a name
// is the sum of its records with that name (an event hook that ran twice
// counts twice); frames without the name count as 0 toward the average.
struct ProfileScopeSummary {
	ProfileName name = 0;
	f64 averageMs = 0.0;
	f64 maxMs = 0.0;
	f64 callsPerFrame = 0.0;
};

// Sorted by averageMs, highest first.
std::vector<ProfileScopeSummary> SummarizeScopes(const Profiler &profiler);

// A counter over the frames that reported it.
struct ProfileCounterSummary {
	ProfileName name = 0;
	f64 average = 0.0;
	f64 max = 0.0;
};

// Sorted by counter name.
std::vector<ProfileCounterSummary> SummarizeCounters(const Profiler &profiler);

f64 FrameDurationMs(const ProfileFrame &frame);
f64 ScopeDurationMs(const ProfileScopeRecord &scope);

// Self time per record of frame (same indices): its duration minus that of
// its direct children.
std::vector<f64> ScopeSelfTimesMs(const ProfileFrame &frame);

// Held frames as Chrome trace event JSON (opens in Perfetto and
// chrome://tracing). Each frame is a "Frame" event with every scope nested
// in it by time, and each counter is a counter track sampled at frame start.
// Timestamps are microseconds from the oldest held frame.
std::string ToChromeTrace(const Profiler &profiler);

} // namespace sowa
