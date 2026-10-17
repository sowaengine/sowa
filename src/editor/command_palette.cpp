#include "editor/command_palette.hpp"

#include <algorithm>

#include "editor/style.hpp"
#include "imgui.h"

namespace sowa::editor {
namespace {

struct PaletteStyle {
	PaletteStyle() {
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {6.f, 6.f});
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {8.f, 5.f});
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {8.f, 6.f});
		ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 4.f);
		ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, {0.f, 0.5f});
	}

	~PaletteStyle() {
		ImGui::PopStyleVar(5);
	}
};

struct Row {
	std::string value;
	std::string label;
	std::string shortcut;
	bool enabled = true;
};

std::vector<Row> rowsFor(const CommandRegistry &commands, const CommandPrompt *prompt,
	const std::string &query, bool playing) {
	std::vector<Row> rows;
	if (prompt) {
		for (const auto &option : prompt->options) {
			if (MatchesCommand(option.label, query)) {
				rows.push_back({option.value, option.label});
			}
		}
	} else {
		for (const auto &command : commands.Search(query)) {
			const bool enabled = command.enabled && (!playing || command.allowWhilePlaying);
			rows.push_back({command.id, command.title, command.shortcut, enabled});
		}
	}
	return rows;
}

} // namespace

void CommandPalette::resetSearch(const std::string &value) {
	_text.assign(value.begin(), value.end());
	_text.resize(std::max<std::size_t>(256, value.size() + 1), '\0');
	_selected = 0;
}

bool CommandPalette::drawSearch(const char *placeholder) {
	_navigation = 0;
	const auto callback = [](ImGuiInputTextCallbackData *data) {
		auto &palette = *static_cast<CommandPalette *>(data->UserData);
		if (data->EventFlag == ImGuiInputTextFlags_CallbackResize) {
			palette._text.resize(data->BufSize);
			data->Buf = palette._text.data();
		} else {
			palette._navigation = data->EventKey == ImGuiKey_UpArrow ? -1 : 1;
		}
		return 0;
	};
	ImGui::SetNextItemWidth(-1.f);
	auto &io = ImGui::GetIO();
	const bool keepActive = io.ConfigInputTextEnterKeepActive;
	io.ConfigInputTextEnterKeepActive = true;
	const bool edited = ImGui::InputTextWithHint("##search", placeholder, _text.data(),
		_text.size(), ImGuiInputTextFlags_CallbackResize | ImGuiInputTextFlags_CallbackHistory,
		callback, this);
	io.ConfigInputTextEnterKeepActive = keepActive;
	return edited;
}

std::optional<std::string> CommandPalette::Draw(const CommandRegistry &commands,
	CommandInputs &inputs, bool playing) {
	if (playing && !inputs.AllowsPlay()) {
		inputs.Cancel();
	}
	bool focus = false;
	if (_openRequested) {
		inputs.Cancel();
		_promptRevision = 0;
		resetSearch();
		_openRequested = false;
		_closeRequested = false;
		focus = true;
	}
	if (inputs.Current() && inputs.Revision() != _promptRevision) {
		_promptRevision = inputs.Revision();
		resetSearch(inputs.Current()->value);
		_closeRequested = false;
		focus = true;
	}
	if (_promptRevision && !inputs.Current()) {
		_closeRequested = true;
	}
	if (focus) {
		ImGui::OpenPopup("Command Palette");
	}

	const PaletteStyle style;
	const auto *viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(
		{viewport->WorkPos.x + viewport->WorkSize.x * 0.5f, viewport->WorkPos.y + 36.f},
		ImGuiCond_Always, {0.5f, 0.f});
	ImGui::SetNextWindowSize({std::min(600.f, viewport->WorkSize.x - 16.f), 0.f});
	if (!ImGui::BeginPopup("Command Palette", ImGuiWindowFlags_NoTitleBar |
												  ImGuiWindowFlags_NoMove |
												  ImGuiWindowFlags_NoSavedSettings)) {
		if (_visible) {
			inputs.Cancel();
		}
		_visible = false;
		return {};
	}
	_visible = true;
	if (_closeRequested) {
		ImGui::CloseCurrentPopup();
		ImGui::EndPopup();
		_visible = _closeRequested = false;
		return {};
	}

	const auto *prompt = inputs.Current();
	const bool textPrompt = prompt && prompt->kind == CommandPrompt::Kind::Text;
	if (prompt) {
		ImGui::TextUnformatted(prompt->title.c_str());
	}
	if (focus || ImGui::IsWindowAppearing()) {
		ImGui::SetKeyboardFocusHere();
	}
	const char *placeholder = prompt ? prompt->placeholder.c_str() : "Type a command...";
	const bool edited = drawSearch(placeholder);
	if (edited) {
		_selected = 0;
	}
	const bool enter = ImGui::IsKeyPressed(ImGuiKey_Enter, false) ||
					   ImGui::IsKeyPressed(ImGuiKey_KeypadEnter, false);
	std::optional<std::string> submitted;

	if (textPrompt) {
		if (enter) {
			submitted = _text.data();
		}
	} else {
		const auto rows = rowsFor(commands, prompt, _text.data(), playing);
		const int count = static_cast<int>(rows.size());
		_selected = count ? (_selected + _navigation + count) % count : 0;
		const float rowHeight = ImGui::GetTextLineHeight() + 6.f;
		const float height = (rowHeight + 2.f) * std::clamp(count, 1, 8) + 8.f;
		// Borderless children otherwise discard padding, leaving the first row
		// and its selection highlight pressed against the search field.
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {8.f, 4.f});
		ImGui::BeginChild("##results", {0.f, height}, ImGuiChildFlags_AlwaysUseWindowPadding);
		ImGui::PopStyleVar();
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {8.f, 2.f});
		if (rows.empty()) {
			ImGui::TextDisabled("No matches.");
		}
		for (int i = 0; i < count; ++i) {
			const auto &row = rows[i];
			ImGui::PushID(i);
			ImGui::BeginDisabled(!row.enabled);
			const bool clicked = ImGui::Selectable(row.label.c_str(), i == _selected,
				ImGuiSelectableFlags_NoAutoClosePopups, {0.f, rowHeight});
			if (row.enabled && (clicked || (enter && i == _selected))) {
				submitted = row.value;
			}
			if (i == _selected && (edited || _navigation)) {
				ImGui::SetScrollHereY();
			}
			ImGui::EndDisabled();
			if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
				if (!row.enabled) {
					ImGui::SetTooltip("Unavailable in the current editor state");
				} else if (!row.shortcut.empty()) {
					ImGui::SetTooltip("%s", row.shortcut.c_str());
				}
			}
			ImGui::PopID();
		}
		ImGui::PopStyleVar();
		ImGui::EndChild();
	}
	if (!inputs.Error().empty()) {
		ImGui::PushStyleColor(ImGuiCol_Text, theme::Rose);
		ImGui::TextWrapped("%s", inputs.Error().c_str());
		ImGui::PopStyleColor();
	}
	const bool cancelled = ImGui::IsKeyPressed(ImGuiKey_Escape);
	if (cancelled || (submitted && !prompt)) {
		ImGui::CloseCurrentPopup();
		_visible = false;
	}
	ImGui::EndPopup();

	if (cancelled) {
		inputs.Cancel();
		return {};
	}
	if (submitted && prompt) {
		inputs.Submit(*submitted);
		_closeRequested = !inputs.Current();
		return {};
	}
	return submitted;
}

} // namespace sowa::editor
