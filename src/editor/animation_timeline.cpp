#include "editor/animation_timeline.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

#include "ecs/world.hpp"
#include "editor/inspector.hpp"
#include "editor/style.hpp"

namespace sowa::editor {
namespace {
constexpr float labelWidth = 220.f;
constexpr float rowHeight = 30.f;
constexpr ImU32 accent = IM_COL32(92, 178, 245, 255);

bool editText(const char *label, std::string &value) {
	std::string edited = value;
	if (!ImGui::InputText(label, &edited, ImGuiInputTextFlags_EnterReturnsTrue)) {
		return false;
	}
	value = std::move(edited);
	return true;
}

bool editValue(AnimationValue &value, const AnimationBinding &binding,
	const AnimationTimeline::ResourceEditor &editResource) {
	constexpr auto flags = ImGuiInputTextFlags_EnterReturnsTrue;
	return std::visit(
		[&](auto &v) {
			using T = std::decay_t<decltype(v)>;
			if constexpr (std::is_same_v<T, bool>) {
				return ImGui::Checkbox("##value", &v);
			} else if constexpr (std::is_same_v<T, i32>) {
				return ImGui::InputScalar("##value", ImGuiDataType_S32, &v, nullptr, nullptr,
					nullptr, flags);
			} else if constexpr (std::is_same_v<T, u32>) {
				return ImGui::InputScalar("##value", ImGuiDataType_U32, &v, nullptr, nullptr,
					nullptr, flags);
			} else if constexpr (std::is_same_v<T, f32>) {
				return ImGui::InputFloat("##value", &v, 0, 0, "%.3f", flags);
			} else if constexpr (std::is_same_v<T, f64>) {
				return ImGui::InputDouble("##value", &v, 0, 0, "%.3f", flags);
			} else if constexpr (std::is_same_v<T, std::string>) {
				return editText("##value", v);
			} else if constexpr (std::is_same_v<T, Vector2>) {
				return ImGui::InputFloat2("##value", &v.x, "%.3f", flags);
			} else if constexpr (std::is_same_v<T, Vector3>) {
				return ImGui::InputFloat3("##value", &v.x, "%.3f", flags);
			} else if constexpr (std::is_same_v<T, Color>) {
				return ImGui::InputFloat4("##value", &v.r, "%.3f", flags);
			} else {
				if (editResource) {
					return editResource(v, binding);
				}
				ImGui::TextDisabled("Resource");
				return false;
			}
		},
		value);
}

void propertyMenu(World &world, Entity root, Entity target, const std::string &path,
	AnimationClip &clip, f64 cursor, bool &changed) {
	if (!ImGui::BeginMenu(target.GetName().c_str())) {
		return;
	}
	auto add = [&](AnimationBinding binding) {
		for (const auto &track : clip.tracks) {
			if (track.binding == binding) {
				return;
			}
		}
		if (auto value = ReadAnimationProperty(world, root, binding)) {
			AnimationTrack track;
			track.binding = std::move(binding);
			track.Insert(cursor, *value);
			clip.tracks.push_back(std::move(track));
			changed = true;
		}
	};
	if (ImGui::MenuItem("Visible")) {
		add({path, "", "isVisible", FieldType::Bool});
	}
	for (auto type : target.GetComponents()) {
		if (!ImGui::BeginMenu(type.Name().c_str())) {
			continue;
		}
		for (const auto &field : type.Fields()) {
			if (field.IsExposed() && ImGui::MenuItem(DisplayName(field.name).c_str())) {
				add({path, type.Name(), field.name, field.type});
			}
		}
		ImGui::EndMenu();
	}
	for (Entity child : target.GetChildren()) {
		propertyMenu(world, root, child, path + "/" + child.GetName(), clip, cursor, changed);
	}
	ImGui::EndMenu();
}

void diamond(ImDrawList &draw, ImVec2 center, ImU32 color) {
	const float radius = 6.f;
	draw.AddQuadFilled({center.x, center.y - radius}, {center.x + radius, center.y},
		{center.x, center.y + radius}, {center.x - radius, center.y}, color);
}
} // namespace

void AnimationTimeline::Reset() {
	_track = -1;
	_key = 0;
	_event = -1;
	_dragKey = 0;
	_dragTrack = -1;
	_dragEvent = -1;
	_error.clear();
}

bool AnimationTimeline::drawAddTrack(World &world, Entity entity, AnimationClip &clip, f64 cursor) {
	bool changed = false;
	if (ImGui::Button("+ Track")) {
		ImGui::OpenPopup("add_track");
	}
	if (const MenuStyle menuStyle; ImGui::BeginPopup("add_track")) {
		propertyMenu(world, entity, entity, ".", clip, cursor, changed);
		if (entity.HasParent()) {
			propertyMenu(world, entity, entity.GetParent(), "..", clip, cursor, changed);
		}
		ImGui::EndPopup();
	}
	if (changed) {
		_track = static_cast<i32>(clip.tracks.size()) - 1;
		_key = clip.tracks.back().Keys().front().id;
		_event = -1;
	}
	return changed;
}

bool AnimationTimeline::Draw(World &world, Entity entity, AnimationClip &clip, f64 &cursor,
	const ResourceEditor &editResource) {
	_scrubbed = false;
	bool changed = drawAddTrack(world, entity, clip, cursor);
	ImGui::SameLine();
	ImGui::BeginDisabled(_track < 0 || _track >= static_cast<i32>(clip.tracks.size()));
	if (ImGui::Button("+ Key")) {
		auto &track = clip.tracks[_track];
		if (auto value = ReadAnimationProperty(world, entity, track.binding)) {
			_key = track.Insert(cursor, *value);
			changed = _key != 0;
		}
	}
	ImGui::EndDisabled();
	ImGui::SameLine();
	if (ImGui::Button("+ Event")) {
		clip.calls.push_back({cursor, ".", "", ""});
		_event = static_cast<i32>(clip.calls.size()) - 1;
		_track = -1;
		_key = 0;
		changed = true;
	}
	ImGui::SameLine();
	ImGui::SetNextItemWidth(80);
	ImGui::DragFloat("Snap", &_snap, .01f, 0, 1, "%.2f s");
	ImGui::SameLine();
	ImGui::SetNextItemWidth(100);
	ImGui::SliderFloat("Zoom", &_zoom, 1, 8, "%.1fx");

	changed |= drawTracks(world, entity, clip, cursor);
	ImGui::Separator();
	changed |= drawSelectedKey(clip, editResource);
	if (!_error.empty()) {
		ImGui::TextColored({1, .5f, .3f, 1}, "%s", _error.c_str());
	}
	return changed;
}

bool AnimationTimeline::drawTracks(World &world, Entity entity, AnimationClip &clip, f64 &cursor) {
	bool changed = false;
	const float width =
		std::max(300.f, ImGui::GetContentRegionAvail().x - labelWidth - 32.f) * _zoom;
	const float pixelsPerSecond = width / static_cast<float>(clip.duration);
	ImGui::BeginChild("timeline", {0, std::max(150.f, ImGui::GetContentRegionAvail().y - 125.f)},
		ImGuiChildFlags_Borders, ImGuiWindowFlags_HorizontalScrollbar);
	const ImVec2 origin = ImGui::GetCursorScreenPos();
	const float startX = origin.x + labelWidth;
	auto timeAt = [&](float x) {
		f64 time = (x - startX) / pixelsPerSecond;
		if (_snap > 0) {
			time = std::round(time / _snap) * _snap;
		}
		return std::clamp(time, 0., clip.duration);
	};
	auto *draw = ImGui::GetWindowDrawList();
	const float totalHeight = rowHeight * (clip.tracks.size() + 2);
	const f64 tickStep = std::pow(10., std::floor(std::log10(clip.duration / 5.)));
	const f64 step = std::max(tickStep, 1e-6);
	for (f64 time = 0; time <= clip.duration && time / step < 100; time += step) {
		const float x = startX + static_cast<float>(time) * pixelsPerSecond;
		draw->AddLine({x, origin.y}, {x, origin.y + totalHeight}, IM_COL32(75, 78, 84, 100));
		char label[32];
		std::snprintf(label, sizeof(label), "%.2g", time);
		draw->AddText({x + 3, origin.y + 4}, IM_COL32(170, 174, 181, 255), label);
	}
	ImGui::SetCursorScreenPos({startX, origin.y});
	ImGui::InvisibleButton("ruler", {width, rowHeight});
	if (ImGui::IsItemActive()) {
		cursor = timeAt(ImGui::GetIO().MousePos.x);
		_scrubbed = true;
	}

	for (i32 index = 0; index < static_cast<i32>(clip.tracks.size()); ++index) {
		auto &track = clip.tracks[index];
		ImGui::PushID(index);
		const float y = origin.y + rowHeight * (index + 1);
		const bool selected = _track == index;
		draw->AddRectFilled({origin.x, y}, {startX + width, y + rowHeight},
			selected ? IM_COL32(48, 64, 82, 150) : IM_COL32(32, 34, 39, 100));
		ImGui::SetCursorScreenPos({origin.x, y});
		const std::string label = track.binding.component.empty()
									  ? "Visible"
									  : track.binding.component + "." + track.binding.property;
		if (ImGui::Selectable(label.c_str(), selected, 0, {labelWidth - 8, rowHeight})) {
			_track = index;
			_key = 0;
			_event = -1;
		}
		if (ImGui::IsItemHovered()) {
			ImGui::SetTooltip("%s", track.binding.path.c_str());
		}
		// Empty track area inserts a key with a double-click.
		ImGui::SetCursorScreenPos({startX, y});
		ImGui::SetNextItemAllowOverlap();
		ImGui::InvisibleButton("row", {width, rowHeight});
		if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
			cursor = timeAt(ImGui::GetIO().MousePos.x);
			if (auto value = ReadAnimationProperty(world, entity, track.binding)) {
				_key = track.Insert(cursor, *value);
				_track = index;
				_event = -1;
				changed = true;
			}
		}
		const auto keys = track.Keys();
		for (const auto &key : keys) {
			ImGui::PushID(static_cast<int>(key.id));
			const bool dragging = _dragKey == key.id && _dragTrack == index;
			const f64 time = dragging ? _dragTime : key.time;
			const ImVec2 center(startX + time * pixelsPerSecond, y + rowHeight / 2);
			ImGui::SetCursorScreenPos({center.x - 8, center.y - 10});
			ImGui::InvisibleButton("key", {16, 20});
			if (ImGui::IsItemActivated()) {
				_track = index;
				_key = key.id;
				_event = -1;
				_dragTrack = index;
				_dragKey = key.id;
				_dragTime = key.time;
				cursor = key.time;
				_scrubbed = true;
			}
			if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
				_dragTime = timeAt(ImGui::GetIO().MousePos.x);
				cursor = _dragTime;
			}
			if (ImGui::IsItemDeactivated() && dragging) {
				if (_dragTime != key.time) {
					changed |= track.Update(key.id, _dragTime, key.value, key.interpolation);
				}
				_dragKey = 0;
				_dragTrack = -1;
			}
			diamond(*draw, center,
				selected && _key == key.id ? accent : IM_COL32(190, 196, 207, 255));
			ImGui::PopID();
		}
		ImGui::PopID();
	}
	const float eventsY = origin.y + rowHeight * (clip.tracks.size() + 1);
	draw->AddText({origin.x + 4, eventsY + 8}, IM_COL32(180, 184, 194, 255), "Events");
	for (i32 index = 0; index < static_cast<i32>(clip.calls.size()); ++index) {
		ImGui::PushID(index);
		ImGui::PushID("event");
		auto &call = clip.calls[index];
		const bool dragging = _dragEvent == index;
		const f64 eventTime = dragging ? _dragTime : call.time;
		const ImVec2 center(startX + eventTime * pixelsPerSecond, eventsY + rowHeight / 2);
		ImGui::SetCursorScreenPos({center.x - 8, center.y - 10});
		ImGui::InvisibleButton("key", {16, 20});
		if (ImGui::IsItemActivated()) {
			_event = index;
			_track = -1;
			_key = 0;
			_dragEvent = index;
			_dragTime = call.time;
			cursor = call.time;
			_scrubbed = true;
		}
		if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
			_dragTime = timeAt(ImGui::GetIO().MousePos.x);
			cursor = _dragTime;
		}
		if (ImGui::IsItemDeactivated() && dragging) {
			if (_dragTime != call.time) {
				call.time = _dragTime;
				changed = true;
			}
			_dragEvent = -1;
		}
		diamond(*draw, center, _event == index ? accent : IM_COL32(224, 179, 96, 255));
		ImGui::PopID();
		ImGui::PopID();
	}
	const float cursorX = startX + cursor * pixelsPerSecond;
	draw->AddLine({cursorX, origin.y}, {cursorX, origin.y + totalHeight}, accent, 2);
	ImGui::SetCursorScreenPos({origin.x, origin.y + totalHeight});
	ImGui::Dummy({labelWidth + width + 16, 1});
	ImGui::EndChild();
	return changed;
}

bool AnimationTimeline::drawSelectedKey(AnimationClip &clip, const ResourceEditor &editResource) {
	bool changed = false;
	if (_event >= 0 && _event < static_cast<i32>(clip.calls.size())) {
		auto &call = clip.calls[_event];
		if (ImGui::BeginTable("event_fields", 4, ImGuiTableFlags_SizingStretchSame)) {
			ImGui::TableNextColumn();
			ImGui::TextDisabled("Time");
			ImGui::SetNextItemWidth(-1);
			f64 time = call.time;
			if (ImGui::InputDouble("##time", &time, 0, 0, "%.3f",
					ImGuiInputTextFlags_EnterReturnsTrue) &&
				std::isfinite(time) && time >= 0 && time <= clip.duration) {
				call.time = time;
				changed = true;
			}
			ImGui::TableNextColumn();
			ImGui::TextDisabled("Target");
			ImGui::SetNextItemWidth(-1);
			changed |= editText("##target", call.path);
			ImGui::TableNextColumn();
			ImGui::TextDisabled("Component");
			ImGui::SetNextItemWidth(-1);
			changed |= editText("##component", call.component);
			ImGui::TableNextColumn();
			ImGui::TextDisabled("Method");
			ImGui::SetNextItemWidth(-1);
			changed |= editText("##method", call.method);
			ImGui::EndTable();
		}
		if (ImGui::Button("Delete event")) {
			clip.calls.erase(clip.calls.begin() + _event);
			_event = -1;
			changed = true;
		}
		return changed;
	}
	if (_track < 0 || _track >= static_cast<i32>(clip.tracks.size())) {
		ImGui::TextDisabled("Select a key to edit it.");
		return false;
	}
	auto &track = clip.tracks[_track];
	if (ImGui::Button("Delete track")) {
		clip.tracks.erase(clip.tracks.begin() + _track);
		Reset();
		return true;
	}
	ImGui::SameLine();
	ImGui::SetNextItemWidth(180);
	changed |= editText("Target", track.binding.path);
	for (auto key : track.Keys()) {
		if (key.id != _key) {
			continue;
		}
		bool edited = false;
		int interpolation = static_cast<int>(key.interpolation);
		if (ImGui::BeginTable("key_fields", 3, ImGuiTableFlags_SizingStretchProp)) {
			ImGui::TableSetupColumn("time", ImGuiTableColumnFlags_WidthFixed, 100);
			ImGui::TableSetupColumn("value", ImGuiTableColumnFlags_WidthStretch, 2);
			ImGui::TableSetupColumn("interpolation", ImGuiTableColumnFlags_WidthStretch, 1);
			ImGui::TableNextColumn();
			ImGui::TextDisabled("Time");
			ImGui::SetNextItemWidth(-1);
			edited = ImGui::InputDouble("##time", &key.time, 0, 0, "%.3f",
				ImGuiInputTextFlags_EnterReturnsTrue);
			ImGui::TableNextColumn();
			ImGui::TextDisabled("Value");
			ImGui::SetNextItemWidth(-1);
			edited |= editValue(key.value, track.binding, editResource);
			ImGui::TableNextColumn();
			ImGui::TextDisabled("Interpolation");
			ImGui::SetNextItemWidth(-1);
			edited |=
				ImGui::Combo("##interpolation", &interpolation, "Step\0Linear\0Smooth\0Angle\0");
			ImGui::EndTable();
		}
		if (edited) {
			if (key.time <= clip.duration && track.Update(key.id, key.time, key.value,
												 static_cast<KeyInterpolation>(interpolation))) {
				changed = true;
				_error.clear();
			} else {
				_error = "Use a free time inside the animation and a valid value.";
			}
		}
		if (ImGui::Button("Delete key")) {
			changed |= track.Remove(key.id);
			_key = 0;
		}
		break;
	}
	return changed;
}

} // namespace sowa::editor
