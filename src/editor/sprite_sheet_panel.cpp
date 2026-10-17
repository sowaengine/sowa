#include "editor/sprite_sheet_panel.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

#include "editor/icons.hpp"
#include "resource/image_texture.hpp"
#include "resource/resource_registry.hpp"

namespace sowa::editor {
namespace {
constexpr int MaxTiles = 65536;
constexpr auto CommitOnEnter = ImGuiInputTextFlags_EnterReturnsTrue;

void tooltip(const char *text) {
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
		ImGui::SetTooltip("%s", text);
	}
}

void image(u64 texture, Rect uv, ImVec2 position, ImVec2 size) {
	if (!texture || uv.w <= 0 || uv.h == 0) {
		return;
	}
	const ImVec2 end{position.x + size.x, position.y + size.y};
	ImGui::GetWindowDrawList()->AddImage(ImTextureRef(texture), position, end,
		{uv.Left(), uv.Top()}, {uv.Right(), uv.Bottom()});
}

bool validSheet(const SpriteAnimation &clip, const ImageTexture *texture) {
	return texture && texture->IsLoaded() && clip.CellRect(0, texture->Size()).w > 0 &&
		   static_cast<i64>(clip.columns) * clip.rows <= MaxTiles;
}

bool axisInputs(const char *label, int &x, int &y) {
	ImGui::TextUnformatted(label);
	ImGui::SetNextItemWidth(-1);
	int values[]{x, y};
	const std::string id = std::string("##") + label;
	if (!ImGui::InputInt2(id.c_str(), values, CommitOnEnter) || values[0] < 0 || values[1] < 0) {
		return false;
	}
	x = values[0];
	y = values[1];
	return true;
}
} // namespace

bool DrawSpriteAnimationPicker(const char *label, const SpriteSheetAnimation *library,
	std::string &animation) {
	bool changed = false;
	ImGui::BeginDisabled(!library || library->Count() == 0);
	const char *preview = animation.empty() ? "Choose animation" : animation.c_str();
	if (ImGui::BeginCombo(label, preview)) {
		if (library) {
			for (const auto &name : library->Names()) {
				if (ImGui::Selectable(name.c_str(), name == animation)) {
					animation = name;
					changed = true;
				}
				if (name == animation) {
					ImGui::SetItemDefaultFocus();
				}
			}
		}
		ImGui::EndCombo();
	}
	ImGui::EndDisabled();
	return changed;
}

void SpriteSheetPanel::resetPreview() {
	_time = 0;
	_playing = false;
}

void SpriteSheetPanel::Select(Entity entity, ResourceID library) {
	if (_entity != entity || _library != library) {
		_entity = entity;
		_library = library;
		_animation.clear();
		_selectedFrame = 0;
		_fitSheet = true;
		resetPreview();
	}
}

bool SpriteSheetPanel::Draw(SpriteSheetAnimation &library, std::string &animation,
	ResourceRegistry &resources, const TexturePicker &pickTexture) {
	bool changed = drawToolbar(library, animation);
	if (_animation != animation) {
		_animation = animation;
		_selectedFrame = 0;
		_fitSheet = true;
		resetPreview();
	}
	ImGui::Separator();
	auto *clip = library.Find(animation);
	if (!clip) {
		ImGui::TextDisabled("Add an animation to get started.");
		return changed;
	}
	if (!ImGui::BeginTable("Sprite sheet layout", 3, ImGuiTableFlags_Resizable)) {
		return changed;
	}
	ImGui::TableSetupColumn("Settings", ImGuiTableColumnFlags_WidthStretch, 1);
	ImGui::TableSetupColumn("Sheet", ImGuiTableColumnFlags_WidthStretch, 2);
	ImGui::TableSetupColumn("Frames", ImGuiTableColumnFlags_WidthStretch, 1);
	ImGui::TableNextRow();
	ImGui::TableNextColumn();
	ImGui::BeginChild("Settings", {0, 0}, ImGuiChildFlags_Borders);
	changed |= drawSettings(*clip, pickTexture);
	ImGui::EndChild();

	const auto texture = resources.Get<ImageTexture>(clip->texture);
	_textureId = texture && resources.Context().device
					 ? resources.Context().device->GetNativeTextureId(texture->Handle())
					 : 0;
	ImGui::TableNextColumn();
	ImGui::BeginChild("Sheet");
	changed |= drawSheet(*clip, texture.get());
	ImGui::EndChild();

	ImGui::TableNextColumn();
	ImGui::BeginChild("Sequence", {0, 0}, ImGuiChildFlags_Borders);
	changed |= drawFrames(*clip, texture.get());
	if (changed) {
		resetPreview();
	}
	drawPreview(*clip, texture.get());
	ImGui::EndChild();
	ImGui::EndTable();
	return changed;
}

bool SpriteSheetPanel::drawToolbar(SpriteSheetAnimation &library, std::string &animation) {
	ImGui::AlignTextToFramePadding();
	ImGui::TextUnformatted("Animation");
	ImGui::SameLine();
	ImGui::SetNextItemWidth(200);
	bool changed = DrawSpriteAnimationPicker("##animation", &library, animation);
	ImGui::SameLine();
	if (ImGui::Button(ICON_FA_PLUS " Add")) {
		int suffix = 1;
		do {
			animation = "Animation " + std::to_string(suffix++);
		} while (library.Has(animation));
		library.Set(animation, {});
		changed = true;
	}
	ImGui::SameLine();
	ImGui::BeginDisabled(!library.Has(animation));
	if (ImGui::Button(ICON_FA_PEN_TO_SQUARE " Rename")) {
		_name = animation;
		ImGui::OpenPopup("Rename animation");
	}
	ImGui::SameLine();
	if (ImGui::Button(ICON_FA_TRASH "##delete_animation")) {
		library.Remove(animation);
		animation = library.Count() ? library.Names().front() : "";
		changed = true;
	}
	tooltip("Delete animation");
	ImGui::EndDisabled();

	if (ImGui::BeginPopup("Rename animation")) {
		if (ImGui::IsWindowAppearing()) {
			ImGui::SetKeyboardFocusHere();
		}
		const bool enter = ImGui::InputText("Name", &_name, CommitOnEnter);
		const bool valid = !_name.empty() && (_name == animation || !library.Has(_name));
		ImGui::BeginDisabled(!valid);
		const bool apply = ImGui::Button("Rename");
		ImGui::EndDisabled();
		if ((enter || apply) && valid) {
			if (_name != animation) {
				const auto clip = *library.Find(animation);
				library.Remove(animation);
				library.Set(_name, clip);
				animation = _name;
				changed = true;
			}
			ImGui::CloseCurrentPopup();
		}
		if (!valid) {
			ImGui::TextDisabled("Use a unique, non-empty name.");
		}
		ImGui::EndPopup();
	}
	return changed;
}

bool SpriteSheetPanel::drawSettings(SpriteAnimation &clip, const TexturePicker &pickTexture) {
	ImGui::SeparatorText("Texture");
	bool changed = pickTexture(clip.texture);
	if (changed) {
		_fitSheet = true;
	}
	ImGui::SeparatorText("Grid");
	ImGui::TextUnformatted("Columns / rows");
	ImGui::SetNextItemWidth(-1);
	int grid[]{clip.columns, clip.rows};
	if (ImGui::InputInt2("##grid", grid, CommitOnEnter) && grid[0] > 0 && grid[1] > 0 &&
		static_cast<i64>(grid[0]) * grid[1] <= MaxTiles) {
		std::vector<i32> frames;
		for (int cell : clip.frames) {
			if (clip.columns > 0 && cell >= 0 && cell % clip.columns < grid[0] &&
				cell / clip.columns < grid[1]) {
				frames.push_back((cell / clip.columns) * grid[0] + cell % clip.columns);
			}
		}
		clip.columns = grid[0];
		clip.rows = grid[1];
		clip.frames = std::move(frames);
		changed = true;
	}
	changed |= axisInputs("Padding (X / Y pixels)", clip.paddingX, clip.paddingY);
	tooltip("Outer border on the left/right and top/bottom of the sheet.");
	changed |= axisInputs("Spacing (X / Y pixels)", clip.spacingX, clip.spacingY);
	tooltip("Horizontal and vertical gaps between tiles.");
	ImGui::SeparatorText("Playback");
	ImGui::TextUnformatted("Frames per second");
	ImGui::SetNextItemWidth(-1);
	float fps = clip.framesPerSecond;
	if (ImGui::InputFloat("##fps", &fps, 0, 0, "%.2f", CommitOnEnter) && std::isfinite(fps) &&
		fps >= 0) {
		clip.framesPerSecond = fps;
		changed = true;
	}
	changed |= ImGui::Checkbox("Loop", &clip.loop);
	return changed;
}

bool SpriteSheetPanel::drawSheet(SpriteAnimation &clip, const ImageTexture *texture) {
	const bool valid = validSheet(clip, texture);
	bool changed = false;
	ImGui::BeginDisabled(!valid);
	if (ImGui::Button("Select all")) {
		clip.frames.clear();
		for (int cell = 0; cell < clip.columns * clip.rows; ++cell) {
			clip.frames.push_back(cell);
		}
		changed = true;
	}
	ImGui::EndDisabled();
	ImGui::SameLine();
	ImGui::BeginDisabled(clip.frames.empty());
	if (ImGui::Button("Clear")) {
		clip.frames.clear();
		changed = true;
	}
	ImGui::EndDisabled();
	ImGui::SameLine();
	if (ImGui::Button("Fit")) {
		_fitSheet = true;
	}
	ImGui::SetNextItemWidth(-1);
	ImGui::SliderFloat("##zoom", &_zoom, .05f, 8.f, "%.2fx", ImGuiSliderFlags_Logarithmic);
	tooltip("Zoom");
	ImGui::PushTextWrapPos();
	ImGui::TextDisabled("Click to select. Shift-click to repeat.");
	ImGui::PopTextWrapPos();
	ImGui::BeginChild("Canvas", {0, 0}, ImGuiChildFlags_Borders,
		ImGuiWindowFlags_HorizontalScrollbar);
	if (!valid) {
		ImGui::TextWrapped(
			texture && texture->IsLoaded()
				? "The grid does not fit this texture. Check columns, rows, padding and spacing."
				: "Choose a texture in Settings to select its frames.");
		ImGui::EndChild();
		return changed;
	}

	const Vector2 size = texture->Size();
	if (_fitSheet) {
		const auto available = ImGui::GetContentRegionAvail();
		_zoom = std::clamp(std::min(available.x / size.x, available.y / size.y), .05f, 8.f);
		_fitSheet = false;
	}
	const auto origin = ImGui::GetCursorScreenPos();
	ImGui::InvisibleButton("sheet", {size.x * _zoom, size.y * _zoom},
		ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);
	const bool hovered = ImGui::IsItemHovered();
	const auto mouse = ImGui::GetMousePos();
	const Rect whole = texture->flipVertically ? Rect(0, 0, 1, 1) : Rect(0, 1, 1, -1);
	image(_textureId, whole, origin, {size.x * _zoom, size.y * _zoom});

	// Build selection labels once, rather than searching every frame for every tile.
	std::vector<int> order(clip.columns * clip.rows, -1);
	for (int i = 0; i < clip.FrameCount(); ++i) {
		const int cell = clip.frames[i];
		if (cell >= 0 && cell < static_cast<int>(order.size()) && order[cell] == -1) {
			order[cell] = i;
		}
	}
	auto *draw = ImGui::GetWindowDrawList();
	for (int cell = 0; cell < static_cast<int>(order.size()); ++cell) {
		const Rect rect = clip.CellRect(cell, size);
		const ImVec2 a{origin.x + rect.x * _zoom, origin.y + rect.y * _zoom};
		const ImVec2 b{a.x + rect.w * _zoom, a.y + rect.h * _zoom};
		if (!ImGui::IsRectVisible(a, b)) {
			continue;
		}
		const bool selected = order[cell] >= 0;
		if (selected) {
			draw->AddRectFilled(a, b, ImGui::GetColorU32(ImGuiCol_Header, .5f));
		}
		draw->AddRect(a, b, ImGui::GetColorU32(selected ? ImGuiCol_HeaderActive : ImGuiCol_Border));
		if (selected && rect.w * _zoom >= 20) {
			const auto number = std::to_string(order[cell] + 1);
			draw->AddText(a, ImGui::GetColorU32(ImGuiCol_Text), number.c_str());
		}
		if (!hovered || mouse.x < a.x || mouse.x >= b.x || mouse.y < a.y || mouse.y >= b.y) {
			continue;
		}
		ImGui::SetTooltip("Tile (%d, %d)\nRight-click to remove all occurrences.",
			cell % clip.columns, cell / clip.columns);
		if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
			if (!selected || ImGui::GetIO().KeyShift) {
				clip.frames.push_back(cell);
				_selectedFrame = clip.FrameCount() - 1;
			} else {
				clip.frames.erase(clip.frames.begin() + order[cell]);
			}
			changed = true;
		} else if (selected && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
			std::erase(clip.frames, cell);
			changed = true;
		}
	}
	ImGui::EndChild();
	return changed;
}

bool SpriteSheetPanel::drawFrames(SpriteAnimation &clip, const ImageTexture *texture) {
	_selectedFrame = std::clamp(_selectedFrame, 0, std::max(0, clip.FrameCount() - 1));
	ImGui::SeparatorText("Frames");
	ImGui::AlignTextToFramePadding();
	ImGui::TextDisabled("%d frames", clip.FrameCount());
	bool changed = false;
	ImGui::BeginDisabled(clip.frames.empty() || _selectedFrame == 0);
	if (ImGui::ArrowButton("up", ImGuiDir_Up)) {
		std::swap(clip.frames[_selectedFrame], clip.frames[_selectedFrame - 1]);
		--_selectedFrame;
		changed = true;
	}
	tooltip("Move frame up");
	ImGui::EndDisabled();
	ImGui::SameLine();
	ImGui::BeginDisabled(clip.frames.empty() || _selectedFrame == clip.FrameCount() - 1);
	if (ImGui::ArrowButton("down", ImGuiDir_Down)) {
		std::swap(clip.frames[_selectedFrame], clip.frames[_selectedFrame + 1]);
		++_selectedFrame;
		changed = true;
	}
	tooltip("Move frame down");
	ImGui::EndDisabled();
	ImGui::SameLine();
	ImGui::BeginDisabled(clip.frames.empty());
	if (ImGui::Button(ICON_FA_TRASH "##delete_frame")) {
		clip.frames.erase(clip.frames.begin() + _selectedFrame);
		_selectedFrame = std::max(0, std::min(_selectedFrame, clip.FrameCount() - 1));
		changed = true;
	}
	tooltip("Delete frame");
	ImGui::EndDisabled();

	// Reserve room for preview; the frame list scrolls independently.
	const float height = std::max(90.f, ImGui::GetContentRegionAvail().y - 200.f);
	ImGui::BeginChild("Frame list", {0, height});
	if (clip.frames.empty()) {
		ImGui::TextWrapped("Select tiles on the sheet to build this animation.");
	}
	constexpr float rowHeight = 42;
	ImGuiListClipper list;
	list.Begin(clip.FrameCount(), rowHeight + ImGui::GetStyle().ItemSpacing.y);
	while (list.Step()) {
		for (int i = list.DisplayStart; i < list.DisplayEnd; ++i) {
			ImGui::PushID(i);
			const auto position = ImGui::GetCursorScreenPos();
			if (ImGui::Selectable("##frame", i == _selectedFrame, 0, {0, rowHeight})) {
				_selectedFrame = i;
				_playing = false;
				_time = clip.framesPerSecond > 0 ? i / double(clip.framesPerSecond) : 0;
			}
			if (texture) {
				const Rect uv = clip.FrameUV(i, texture->Size(), texture->flipVertically);
				const Rect cell = clip.CellRect(clip.frames[i], texture->Size());
				if (cell.w > 0 && cell.h > 0) {
					const float scale = 34.f / std::max(cell.w, cell.h);
					image(_textureId, uv, {position.x + 4, position.y + 4},
						{cell.w * scale, cell.h * scale});
				}
			}
			const int cell = clip.frames[i];
			char label[80];
			std::snprintf(label, sizeof(label), "%d    (%d, %d)", i + 1,
				clip.columns > 0 ? cell % clip.columns : 0,
				clip.columns > 0 ? cell / clip.columns : 0);
			ImGui::GetWindowDrawList()->AddText({position.x + 46, position.y + 12},
				ImGui::GetColorU32(ImGuiCol_Text), label);
			ImGui::PopID();
		}
	}
	ImGui::EndChild();
	return changed;
}

void SpriteSheetPanel::drawPreview(const SpriteAnimation &clip, const ImageTexture *texture) {
	ImGui::SeparatorText("Preview");
	const bool valid = validSheet(clip, texture) && clip.IsValid();
	ImGui::BeginDisabled(!valid);
	if (ImGui::Button(_playing ? ICON_FA_PAUSE : ICON_FA_PLAY)) {
		if (!_playing && _time >= clip.Duration()) {
			_time = 0;
		} else if (!_playing && _time == 0 && clip.framesPerSecond > 0) {
			_time = _selectedFrame / double(clip.framesPerSecond);
		}
		_playing = !_playing;
	}
	tooltip(_playing ? "Pause" : "Play");
	ImGui::SameLine();
	if (ImGui::Button(ICON_FA_STOP)) {
		resetPreview();
		_selectedFrame = 0;
	}
	tooltip("Stop");
	ImGui::EndDisabled();
	if (!valid) {
		return;
	}
	if (_playing && clip.Duration() > 0) {
		_time += ImGui::GetIO().DeltaTime;
		if (_time >= clip.Duration()) {
			_time = clip.loop ? std::fmod(_time, clip.Duration()) : clip.Duration();
			_playing = clip.loop;
		}
	}
	int frame = _selectedFrame;
	if (clip.Duration() > 0 && (_playing || _time > 0)) {
		frame =
			static_cast<int>(std::min(_time * clip.framesPerSecond, double(clip.FrameCount() - 1)));
	}
	frame = std::clamp(frame, 0, clip.FrameCount() - 1);
	ImGui::SameLine();
	ImGui::TextDisabled("%d / %d", frame + 1, clip.FrameCount());
	const Rect cell = clip.CellRect(clip.frames[frame], texture->Size());
	const float width = ImGui::GetContentRegionAvail().x;
	const float scale = std::min(std::max(1.f, width - 16) / cell.w, 110.f / cell.h);
	const ImVec2 size{cell.w * scale, cell.h * scale};
	const auto position = ImGui::GetCursorScreenPos();
	ImGui::Dummy({width, 118});
	ImGui::GetWindowDrawList()->AddRectFilled(position, {position.x + width, position.y + 118},
		ImGui::GetColorU32(ImGuiCol_FrameBg), 4);
	image(_textureId, clip.FrameUV(frame, texture->Size(), texture->flipVertically),
		{position.x + (width - size.x) / 2, position.y + (118 - size.y) / 2}, size);
}

} // namespace sowa::editor
