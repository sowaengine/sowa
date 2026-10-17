#include "editor/profiler_panel.hpp"

#include <algorithm>
#include <climits>
#include <fstream>
#include <map>
#include <system_error>
#include <vector>

#include <fmt/format.h>
#include <imgui.h>

#include "core/profiler_report.hpp"
#include "editor/style.hpp"

namespace sowa::editor {

namespace {

// Frame budgets drawn on the graph: 60 and 30 frames per second.
constexpr f64 kBudgetMs = 1000.0 / 60.0;
constexpr f64 kSlowMs = 1000.0 / 30.0;
constexpr float kGraphHeight = 90.f;

ImVec4 frameColor(f64 ms) {
	if (ms <= kBudgetMs) {
		return theme::Green;
	}
	return ms <= kSlowMs ? theme::Amber : theme::Rose;
}

} // namespace

std::size_t ResolveProfilerFrame(const Profiler &profiler, u64 frameIndex) {
	const std::size_t count = profiler.FrameCount();
	if (count == 0) {
		return 0;
	}
	if (frameIndex != 0) {
		for (std::size_t i = 0; i < count; ++i) {
			if (profiler.FrameAt(i).index == frameIndex) {
				return i;
			}
		}
	}
	return count - 1;
}

std::filesystem::path ProfilerTracePath(const std::filesystem::path &projectRoot, std::time_t now) {
	std::tm local{};
#ifdef _WIN32
	localtime_s(&local, &now);
#else
	localtime_r(&now, &local);
#endif
	char stamp[32];
	std::strftime(stamp, sizeof(stamp), "%Y%m%d-%H%M%S", &local);
	return projectRoot / ".sowa" / "profiler" / (std::string("trace-") + stamp + ".json");
}

std::string WriteProfilerTrace(const Profiler &profiler, const std::filesystem::path &path) {
	std::error_code ec;
	std::filesystem::create_directories(path.parent_path(), ec);
	if (ec) {
		return fmt::format("cannot create {}: {}", path.parent_path().string(), ec.message());
	}
	std::ofstream file(path, std::ios::binary | std::ios::trunc);
	if (!file) {
		return fmt::format("cannot write {}", path.string());
	}
	file << ToChromeTrace(profiler);
	file.close();
	if (!file) {
		return fmt::format("failed while writing {}", path.string());
	}
	return {};
}

void ProfilerPanel::Draw(bool &open, const std::filesystem::path &projectRoot) {
	Profiler &profiler = Profiler::Main();
	profiler.SetEnabled(open);
	if (!open) {
		return;
	}

	ImGui::SetNextWindowSize({760, 520}, ImGuiCond_FirstUseEver);
	if (!ImGui::Begin("Profiler", &open)) {
		ImGui::End();
		return;
	}

	drawToolbar(profiler, projectRoot);
	if (profiler.FrameCount() == 0) {
		ImGui::TextDisabled(profiler.IsFrozen() ? "Frozen with no frames recorded."
												: "Waiting for the first frame...");
		ImGui::End();
		return;
	}

	const std::size_t selected = ResolveProfilerFrame(profiler, _selectedFrame);
	drawFrameGraph(profiler, selected);
	const ProfileFrame &frame = profiler.FrameAt(selected);

	if (ImGui::BeginTabBar("##profiler_tabs")) {
		if (ImGui::BeginTabItem("Frame")) {
			drawFrameTree(profiler, frame);
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Scopes")) {
			drawScopeTable(profiler);
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Counters")) {
			drawCounterTable(profiler, frame);
			ImGui::EndTabItem();
		}
		ImGui::EndTabBar();
	}
	ImGui::End();
}

void ProfilerPanel::drawToolbar(Profiler &profiler, const std::filesystem::path &projectRoot) {
	const bool frozen = profiler.IsFrozen();
	if (ImGui::Button(frozen ? "Resume" : "Freeze")) {
		profiler.SetFrozen(!frozen);
		if (frozen) {
			_selectedFrame = 0;
		}
	}
	if (ImGui::IsItemHovered()) {
		ImGui::SetTooltip(frozen ? "Record new frames again" : "Hold the recorded history");
	}
	ImGui::SameLine();
	if (ImGui::Button("Clear")) {
		profiler.Clear();
		_selectedFrame = 0;
	}
	ImGui::SameLine();
	ImGui::BeginDisabled(projectRoot.empty() || profiler.FrameCount() == 0);
	if (ImGui::Button("Export Trace")) {
		_exportedPath = ProfilerTracePath(projectRoot, std::time(nullptr));
		const std::string error = WriteProfilerTrace(profiler, _exportedPath);
		_exportFailed = !error.empty();
		_exportStatus = _exportFailed ? "Export failed: " + error : _exportedPath.string();
	}
	ImGui::EndDisabled();
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
		ImGui::SetTooltip("Write the held frames as a Chrome trace into .sowa/profiler.\n"
						  "Open it at ui.perfetto.dev or chrome://tracing.");
	}

	if (profiler.FrameCount() > 0) {
		f64 sum = 0.0;
		f64 worst = 0.0;
		for (std::size_t i = 0; i < profiler.FrameCount(); ++i) {
			const f64 ms = FrameDurationMs(profiler.FrameAt(i));
			sum += ms;
			worst = std::max(worst, ms);
		}
		const f64 average = sum / static_cast<f64>(profiler.FrameCount());
		ImGui::SameLine();
		ImGui::TextDisabled("%zu frames  |  avg %.2f ms (%.0f fps)  |  max %.2f ms",
			profiler.FrameCount(), average, average > 0.0 ? 1000.0 / average : 0.0, worst);
	}

	if (!_exportStatus.empty()) {
		if (_exportFailed) {
			ImGui::TextColored(theme::Rose, "%s", _exportStatus.c_str());
		} else {
			ImGui::TextDisabled("Trace saved: %s", _exportStatus.c_str());
			ImGui::SameLine();
			if (ImGui::SmallButton("Copy Path")) {
				ImGui::SetClipboardText(_exportStatus.c_str());
			}
		}
	}
}

void ProfilerPanel::drawFrameGraph(Profiler &profiler, std::size_t selected) {
	const std::size_t count = profiler.FrameCount();
	const float width = std::max(ImGui::GetContentRegionAvail().x, 1.f);
	const float barWidth = std::max(width / static_cast<float>(profiler.HistorySize()), 1.f);
	const std::size_t visible =
		std::min(count, static_cast<std::size_t>(std::max(width / barWidth, 1.f)));
	const std::size_t first = count - visible;

	f64 scaleMs = kSlowMs * 1.25;
	for (std::size_t i = first; i < count; ++i) {
		scaleMs = std::max(scaleMs, FrameDurationMs(profiler.FrameAt(i)));
	}

	const ImVec2 origin = ImGui::GetCursorScreenPos();
	ImGui::InvisibleButton("##frame_graph", ImVec2(width, kGraphHeight));
	const bool hovered = ImGui::IsItemHovered();
	ImDrawList *draw = ImGui::GetWindowDrawList();
	const ImVec2 max(origin.x + width, origin.y + kGraphHeight);
	draw->AddRectFilled(origin, max, ImGui::GetColorU32(ImGuiCol_FrameBg));

	const auto yFor = [&](f64 ms) {
		return max.y - static_cast<float>(ms / scaleMs) * kGraphHeight;
	};
	// Bars are right-aligned so the newest frame is always at the right edge.
	const float left = max.x - barWidth * static_cast<float>(visible);
	std::size_t hoveredFrame = count;
	for (std::size_t i = first; i < count; ++i) {
		const f64 ms = FrameDurationMs(profiler.FrameAt(i));
		const float x0 = left + barWidth * static_cast<float>(i - first);
		const float x1 = x0 + std::max(barWidth - (barWidth > 3.f ? 1.f : 0.f), 1.f);
		ImVec4 color = frameColor(ms);
		if (i == selected) {
			color = theme::Accent;
			draw->AddRectFilled(ImVec2(x0, origin.y), ImVec2(x1, max.y),
				ImGui::GetColorU32(ImVec4(color.x, color.y, color.z, 0.2f)));
		}
		draw->AddRectFilled(ImVec2(x0, yFor(ms)), ImVec2(x1, max.y), ImGui::GetColorU32(color));
		if (hovered && ImGui::GetIO().MousePos.x >= x0 &&
			ImGui::GetIO().MousePos.x < x0 + barWidth) {
			hoveredFrame = i;
		}
	}
	for (const f64 budget : {kBudgetMs, kSlowMs}) {
		const float y = yFor(budget);
		draw->AddLine(ImVec2(origin.x, y), ImVec2(max.x, y),
			ImGui::GetColorU32(ImVec4(1.f, 1.f, 1.f, 0.25f)));
		draw->AddText(ImVec2(origin.x + 4.f, y - ImGui::GetTextLineHeight()),
			ImGui::GetColorU32(ImGuiCol_TextDisabled),
			budget == kBudgetMs ? "16.7 ms (60 fps)" : "33.3 ms (30 fps)");
	}

	if (hoveredFrame < count) {
		const ProfileFrame &frame = profiler.FrameAt(hoveredFrame);
		ImGui::SetTooltip("Frame %llu: %.2f ms\nClick to inspect (freezes recording)",
			static_cast<unsigned long long>(frame.index), FrameDurationMs(frame));
		if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
			// Freezing keeps the selected frame from scrolling out of the
			// history while it is inspected.
			profiler.SetFrozen(true);
			_selectedFrame = frame.index;
		}
	}
}

void ProfilerPanel::drawFrameTree(const Profiler &profiler, const ProfileFrame &frame) {
	const f64 frameMs = FrameDurationMs(frame);
	ImGui::Text("Frame %llu: %.2f ms", static_cast<unsigned long long>(frame.index), frameMs);
	const std::vector<f64> self = ScopeSelfTimesMs(frame);

	constexpr ImGuiTableFlags flags = ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg |
									  ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY;
	if (!ImGui::BeginTable("##frame_tree", 4, flags)) {
		return;
	}
	ImGui::TableSetupScrollFreeze(0, 1);
	ImGui::TableSetupColumn("Scope", ImGuiTableColumnFlags_WidthStretch);
	ImGui::TableSetupColumn("Total ms", ImGuiTableColumnFlags_WidthFixed, 80.f);
	ImGui::TableSetupColumn("Self ms", ImGuiTableColumnFlags_WidthFixed, 80.f);
	ImGui::TableSetupColumn("% frame", ImGuiTableColumnFlags_WidthFixed, 70.f);
	ImGui::TableHeadersRow();

	// Records are in begin order with their depth: walk them as a tree,
	// skipping the subtree of a collapsed node. A repeated name is told apart
	// by its occurrence, so expanded state stays put from frame to frame.
	std::map<ProfileName, int> occurrences;
	u32 pushed = 0;
	u32 skipBelow = UINT_MAX;
	for (std::size_t i = 0; i < frame.scopes.size(); ++i) {
		const ProfileScopeRecord &scope = frame.scopes[i];
		const int occurrence = occurrences[scope.name]++;
		if (scope.depth > skipBelow) {
			continue;
		}
		skipBelow = UINT_MAX;
		while (pushed > scope.depth) {
			ImGui::TreePop();
			--pushed;
		}

		const bool hasChildren =
			i + 1 < frame.scopes.size() && frame.scopes[i + 1].depth > scope.depth;
		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);
		ImGui::PushID(static_cast<int>(scope.name));
		ImGui::PushID(occurrence);
		const std::string name(profiler.NameOf(scope.name));
		ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_SpanFullWidth;
		nodeFlags |= hasChildren ? ImGuiTreeNodeFlags_DefaultOpen
								 : ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
		const bool open = ImGui::TreeNodeEx("##scope", nodeFlags, "%s", name.c_str());
		ImGui::PopID();
		ImGui::PopID();
		if (hasChildren) {
			if (open) {
				++pushed;
			} else {
				skipBelow = scope.depth;
			}
		}

		const f64 ms = ScopeDurationMs(scope);
		ImGui::TableSetColumnIndex(1);
		ImGui::Text("%.3f", ms);
		ImGui::TableSetColumnIndex(2);
		ImGui::Text("%.3f", self[i]);
		ImGui::TableSetColumnIndex(3);
		ImGui::Text("%.1f%%", frameMs > 0.0 ? ms / frameMs * 100.0 : 0.0);
	}
	while (pushed > 0) {
		ImGui::TreePop();
		--pushed;
	}
	ImGui::EndTable();
}

void ProfilerPanel::drawScopeTable(const Profiler &profiler) {
	std::vector<ProfileScopeSummary> summaries = SummarizeScopes(profiler);
	constexpr ImGuiTableFlags flags = ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg |
									  ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY |
									  ImGuiTableFlags_Sortable;
	if (!ImGui::BeginTable("##scope_table", 4, flags)) {
		return;
	}
	ImGui::TableSetupScrollFreeze(0, 1);
	ImGui::TableSetupColumn("Scope", ImGuiTableColumnFlags_WidthStretch);
	ImGui::TableSetupColumn("Avg ms",
		ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_DefaultSort |
			ImGuiTableColumnFlags_PreferSortDescending,
		80.f);
	ImGui::TableSetupColumn("Max ms",
		ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_PreferSortDescending, 80.f);
	ImGui::TableSetupColumn("Calls / frame",
		ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_PreferSortDescending, 100.f);
	ImGui::TableHeadersRow();

	if (const ImGuiTableSortSpecs *specs = ImGui::TableGetSortSpecs();
		specs != nullptr && specs->SpecsCount > 0) {
		const ImGuiTableColumnSortSpecs &spec = specs->Specs[0];
		const bool ascending = spec.SortDirection == ImGuiSortDirection_Ascending;
		const auto key = [&](const ProfileScopeSummary &s) {
			switch (spec.ColumnIndex) {
			case 2:
				return s.maxMs;
			case 3:
				return s.callsPerFrame;
			default:
				return s.averageMs;
			}
		};
		std::stable_sort(summaries.begin(), summaries.end(),
			[&](const ProfileScopeSummary &a, const ProfileScopeSummary &b) {
				if (spec.ColumnIndex == 0) {
					return ascending ? profiler.NameOf(a.name) < profiler.NameOf(b.name)
									 : profiler.NameOf(a.name) > profiler.NameOf(b.name);
				}
				return ascending ? key(a) < key(b) : key(a) > key(b);
			});
	}

	for (const ProfileScopeSummary &summary : summaries) {
		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);
		const std::string name(profiler.NameOf(summary.name));
		ImGui::TextUnformatted(name.c_str());
		ImGui::TableSetColumnIndex(1);
		ImGui::Text("%.3f", summary.averageMs);
		ImGui::TableSetColumnIndex(2);
		ImGui::Text("%.3f", summary.maxMs);
		ImGui::TableSetColumnIndex(3);
		ImGui::Text("%.2f", summary.callsPerFrame);
	}
	ImGui::EndTable();
}

void ProfilerPanel::drawCounterTable(const Profiler &profiler, const ProfileFrame &frame) {
	constexpr ImGuiTableFlags flags = ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg |
									  ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY;
	if (!ImGui::BeginTable("##counter_table", 4, flags)) {
		return;
	}
	ImGui::TableSetupScrollFreeze(0, 1);
	ImGui::TableSetupColumn("Counter", ImGuiTableColumnFlags_WidthStretch);
	ImGui::TableSetupColumn("This frame", ImGuiTableColumnFlags_WidthFixed, 100.f);
	ImGui::TableSetupColumn("Avg", ImGuiTableColumnFlags_WidthFixed, 100.f);
	ImGui::TableSetupColumn("Max", ImGuiTableColumnFlags_WidthFixed, 100.f);
	ImGui::TableHeadersRow();

	for (const ProfileCounterSummary &summary : SummarizeCounters(profiler)) {
		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);
		const std::string name(profiler.NameOf(summary.name));
		ImGui::TextUnformatted(name.c_str());
		ImGui::TableSetColumnIndex(1);
		const auto current = std::find_if(frame.counters.begin(), frame.counters.end(),
			[&](const ProfileCounterRecord &counter) { return counter.name == summary.name; });
		if (current != frame.counters.end()) {
			ImGui::Text("%.6g", current->value);
		} else {
			ImGui::TextDisabled("-");
		}
		ImGui::TableSetColumnIndex(2);
		ImGui::Text("%.6g", summary.average);
		ImGui::TableSetColumnIndex(3);
		ImGui::Text("%.6g", summary.max);
	}
	ImGui::EndTable();
}

} // namespace sowa::editor
