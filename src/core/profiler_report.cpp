#include "core/profiler_report.hpp"

#include <algorithm>
#include <map>
#include <string_view>

#include <fmt/format.h>

namespace sowa {

namespace {

constexpr f64 kNsPerMs = 1'000'000.0;

void appendJsonString(std::string &out, std::string_view text) {
	out.push_back('"');
	for (const char c : text) {
		switch (c) {
		case '"':
			out += "\\\"";
			break;
		case '\\':
			out += "\\\\";
			break;
		case '\n':
			out += "\\n";
			break;
		case '\t':
			out += "\\t";
			break;
		default:
			if (static_cast<unsigned char>(c) < 0x20) {
				out += fmt::format("\\u{:04x}", static_cast<unsigned>(c));
			} else {
				out.push_back(c);
			}
		}
	}
	out.push_back('"');
}

} // namespace

f64 FrameDurationMs(const ProfileFrame &frame) {
	return static_cast<f64>(frame.endNs - frame.beginNs) / kNsPerMs;
}

f64 ScopeDurationMs(const ProfileScopeRecord &scope) {
	return static_cast<f64>(scope.endNs - scope.beginNs) / kNsPerMs;
}

std::vector<ProfileScopeSummary> SummarizeScopes(const Profiler &profiler) {
	struct Totals {
		f64 sumMs = 0.0;
		f64 maxMs = 0.0;
		u64 calls = 0;
	};

	std::map<ProfileName, Totals> totals;
	std::map<ProfileName, f64> frameMs;

	const std::size_t frames = profiler.FrameCount();
	for (std::size_t i = 0; i < frames; ++i) {
		frameMs.clear();
		for (const ProfileScopeRecord &scope : profiler.FrameAt(i).scopes) {
			frameMs[scope.name] += ScopeDurationMs(scope);
			++totals[scope.name].calls;
		}
		for (const auto &[name, ms] : frameMs) {
			Totals &total = totals[name];
			total.sumMs += ms;
			total.maxMs = std::max(total.maxMs, ms);
		}
	}

	std::vector<ProfileScopeSummary> summaries;
	summaries.reserve(totals.size());
	for (const auto &[name, total] : totals) {
		summaries.push_back(ProfileScopeSummary{name, total.sumMs / static_cast<f64>(frames),
			total.maxMs, static_cast<f64>(total.calls) / static_cast<f64>(frames)});
	}
	std::stable_sort(summaries.begin(), summaries.end(),
		[](const ProfileScopeSummary &a, const ProfileScopeSummary &b) {
			return a.averageMs > b.averageMs;
		});
	return summaries;
}

std::vector<ProfileCounterSummary> SummarizeCounters(const Profiler &profiler) {
	struct Totals {
		f64 sum = 0.0;
		f64 max = 0.0;
		u64 frames = 0;
	};

	std::map<ProfileName, Totals> totals;
	for (std::size_t i = 0; i < profiler.FrameCount(); ++i) {
		for (const ProfileCounterRecord &counter : profiler.FrameAt(i).counters) {
			Totals &total = totals[counter.name];
			total.max = total.frames == 0 ? counter.value : std::max(total.max, counter.value);
			total.sum += counter.value;
			++total.frames;
		}
	}

	std::vector<ProfileCounterSummary> summaries;
	summaries.reserve(totals.size());
	for (const auto &[name, total] : totals) {
		summaries.push_back(
			ProfileCounterSummary{name, total.sum / static_cast<f64>(total.frames), total.max});
	}
	std::sort(summaries.begin(), summaries.end(),
		[&profiler](const ProfileCounterSummary &a, const ProfileCounterSummary &b) {
			return profiler.NameOf(a.name) < profiler.NameOf(b.name);
		});
	return summaries;
}

std::vector<f64> ScopeSelfTimesMs(const ProfileFrame &frame) {
	std::vector<f64> self(frame.scopes.size());
	// Records are in begin order, so the open parents of record i are the
	// stack entries below its depth.
	std::vector<std::size_t> parents;
	for (std::size_t i = 0; i < frame.scopes.size(); ++i) {
		const ProfileScopeRecord &scope = frame.scopes[i];
		self[i] = ScopeDurationMs(scope);
		parents.resize(std::min<std::size_t>(parents.size(), scope.depth));
		if (!parents.empty()) {
			self[parents.back()] -= self[i];
		}
		parents.push_back(i);
	}
	return self;
}

std::string ToChromeTrace(const Profiler &profiler) {
	std::string out = R"({"displayTimeUnit":"ms","traceEvents":[)";
	const std::size_t frames = profiler.FrameCount();
	const u64 origin = frames > 0 ? profiler.FrameAt(0).beginNs : 0;
	const auto micros = [origin](u64 ns) { return static_cast<f64>(ns - origin) / 1000.0; };
	bool first = true;
	const auto beginEvent = [&] {
		if (!first) {
			out.push_back(',');
		}
		first = false;
	};

	for (std::size_t i = 0; i < frames; ++i) {
		const ProfileFrame &frame = profiler.FrameAt(i);
		beginEvent();
		out += fmt::format(R"({{"name":"Frame","ph":"X","pid":1,"tid":1,"ts":{:.3f},"dur":{:.3f},)"
						   R"("args":{{"index":{}}}}})",
			micros(frame.beginNs), static_cast<f64>(frame.endNs - frame.beginNs) / 1000.0,
			frame.index);
		for (const ProfileScopeRecord &scope : frame.scopes) {
			beginEvent();
			out += R"({"name":)";
			appendJsonString(out, profiler.NameOf(scope.name));
			out += fmt::format(R"(,"ph":"X","pid":1,"tid":1,"ts":{:.3f},"dur":{:.3f}}})",
				micros(scope.beginNs), static_cast<f64>(scope.endNs - scope.beginNs) / 1000.0);
		}
		for (const ProfileCounterRecord &counter : frame.counters) {
			beginEvent();
			out += R"({"name":)";
			appendJsonString(out, profiler.NameOf(counter.name));
			out += fmt::format(R"(,"ph":"C","pid":1,"ts":{:.3f},"args":{{"value":{}}}}})",
				micros(frame.beginNs), counter.value);
		}
	}
	out += "]}";
	return out;
}

} // namespace sowa
