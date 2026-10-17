#include "editor/style.hpp"

#include "imgui_internal.h"

namespace sowa::editor {

void ApplyStyle(ImGuiStyle &style) {
	style.WindowMenuButtonPosition = ImGuiDir_None;
	style.WindowPadding = ImVec2(10.f, 10.f);
	style.FramePadding = ImVec2(8.f, 4.f);
	style.CellPadding = ImVec2(6.f, 4.f);
	style.ItemSpacing = ImVec2(8.f, 6.f);
	style.ItemInnerSpacing = ImVec2(6.f, 4.f);
	style.IndentSpacing = 20.f;
	style.ScrollbarSize = 12.f;
	style.GrabMinSize = 10.f;
	style.WindowBorderSize = 1.f;
	style.ChildBorderSize = 1.f;
	style.PopupBorderSize = 1.f;
	style.FrameBorderSize = 0.f;
	style.TabBorderSize = 0.f;
	style.WindowRounding = 0.f;
	style.ChildRounding = 3.f;
	style.FrameRounding = 3.f;
	style.PopupRounding = 6.f;
	style.ScrollbarRounding = 6.f;
	style.GrabRounding = 2.f;
	style.TabRounding = 3.f;
	style.WindowTitleAlign = ImVec2(0.f, 0.5f);
	style.ColorButtonPosition = ImGuiDir_Right;
	style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
	style.SelectableTextAlign = ImVec2(0.f, 0.5f);
	style.SeparatorTextBorderSize = 1.f;
	style.SeparatorTextPadding = ImVec2(8.f, 6.f);
	style.DockingSeparatorSize = 4.f;
	style.TreeLinesSize = 1.f;
	style.TreeLinesRounding = 2.f;

	auto *c = style.Colors;
	c[ImGuiCol_Text] = ImVec4(0.84f, 0.86f, 0.91f, 1.f);
	c[ImGuiCol_TextDisabled] = ImVec4(0.52f, 0.56f, 0.58f, 1.f);
	c[ImGuiCol_WindowBg] = ImVec4(0.075f, 0.102f, 0.118f, 1.f);
	c[ImGuiCol_ChildBg] = ImVec4(0.063f, 0.086f, 0.102f, 1.f);
	c[ImGuiCol_PopupBg] = ImVec4(0.090f, 0.122f, 0.141f, 1.f);
	c[ImGuiCol_Border] = ImVec4(0.141f, 0.196f, 0.220f, 1.f);
	c[ImGuiCol_BorderShadow] = ImVec4(0.f, 0.f, 0.f, 0.f);
	c[ImGuiCol_FrameBg] = ImVec4(0.047f, 0.067f, 0.082f, 1.f);
	c[ImGuiCol_FrameBgHovered] = ImVec4(0.110f, 0.169f, 0.196f, 1.f);
	c[ImGuiCol_FrameBgActive] = ImVec4(0.078f, 0.220f, 0.251f, 1.f);
	c[ImGuiCol_TitleBg] = ImVec4(0.035f, 0.055f, 0.067f, 1.f);
	c[ImGuiCol_TitleBgActive] = c[ImGuiCol_TitleBg];
	c[ImGuiCol_TitleBgCollapsed] = c[ImGuiCol_TitleBg];
	c[ImGuiCol_MenuBarBg] = ImVec4(0.043f, 0.067f, 0.082f, 1.f);
	c[ImGuiCol_ScrollbarBg] = ImVec4(0.f, 0.f, 0.f, 0.f);
	c[ImGuiCol_ScrollbarGrab] = ImVec4(0.176f, 0.243f, 0.271f, 1.f);
	c[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.255f, 0.341f, 0.373f, 1.f);
	c[ImGuiCol_ScrollbarGrabActive] = theme::Accent;
	c[ImGuiCol_CheckMark] = theme::Accent;
	c[ImGuiCol_SliderGrab] = theme::Accent;
	c[ImGuiCol_SliderGrabActive] = ImVec4(0.45f, 0.87f, 0.92f, 1.f);
	c[ImGuiCol_Button] = ImVec4(0.118f, 0.169f, 0.192f, 1.f);
	c[ImGuiCol_ButtonHovered] = ImVec4(0.165f, 0.247f, 0.278f, 1.f);
	c[ImGuiCol_ButtonActive] = ImVec4(0.086f, 0.290f, 0.329f, 1.f);
	c[ImGuiCol_Header] = ImVec4(0.078f, 0.227f, 0.259f, 1.f);
	c[ImGuiCol_HeaderHovered] = ImVec4(0.094f, 0.188f, 0.216f, 1.f);
	c[ImGuiCol_HeaderActive] = ImVec4(0.102f, 0.278f, 0.314f, 1.f);
	c[ImGuiCol_Separator] = c[ImGuiCol_Border];
	c[ImGuiCol_SeparatorHovered] = theme::Accent;
	c[ImGuiCol_SeparatorActive] = theme::Accent;
	c[ImGuiCol_ResizeGrip] = ImVec4(0.f, 0.f, 0.f, 0.f);
	c[ImGuiCol_ResizeGripHovered] = c[ImGuiCol_ButtonHovered];
	c[ImGuiCol_ResizeGripActive] = theme::Accent;
	c[ImGuiCol_Tab] = c[ImGuiCol_TitleBg];
	c[ImGuiCol_TabHovered] = c[ImGuiCol_ButtonHovered];
	c[ImGuiCol_TabSelected] = ImVec4(0.067f, 0.180f, 0.208f, 1.f);
	c[ImGuiCol_TabSelectedOverline] = theme::Accent;
	c[ImGuiCol_TabDimmed] = c[ImGuiCol_TitleBg];
	c[ImGuiCol_TabDimmedSelected] = ImVec4(0.086f, 0.137f, 0.157f, 1.f);
	c[ImGuiCol_TabDimmedSelectedOverline] = ImVec4(0.188f, 0.431f, 0.471f, 1.f);
	c[ImGuiCol_DockingPreview] = ImVec4(theme::Accent.x, theme::Accent.y, theme::Accent.z, 0.35f);
	c[ImGuiCol_DockingEmptyBg] = c[ImGuiCol_TitleBg];
	c[ImGuiCol_PlotLines] = theme::Blue;
	c[ImGuiCol_PlotLinesHovered] = theme::Accent;
	c[ImGuiCol_PlotHistogram] = theme::Amber;
	c[ImGuiCol_PlotHistogramHovered] = theme::Rose;
	c[ImGuiCol_TableHeaderBg] = c[ImGuiCol_Button];
	c[ImGuiCol_TableBorderStrong] = c[ImGuiCol_Border];
	c[ImGuiCol_TableBorderLight] = ImVec4(0.118f, 0.165f, 0.188f, 0.6f);
	c[ImGuiCol_TableRowBg] = ImVec4(0.f, 0.f, 0.f, 0.f);
	c[ImGuiCol_TableRowBgAlt] = ImVec4(0.35f, 0.65f, 0.75f, 0.025f);
	c[ImGuiCol_TreeLines] = ImVec4(0.176f, 0.243f, 0.271f, 0.65f);
	c[ImGuiCol_TextSelectedBg] = ImVec4(theme::Accent.x, theme::Accent.y, theme::Accent.z, 0.4f);
	c[ImGuiCol_DragDropTarget] = theme::Accent;
	c[ImGuiCol_NavHighlight] = theme::Accent;
	c[ImGuiCol_NavWindowingHighlight] = ImVec4(0.70f, 0.90f, 0.93f, 0.7f);
	c[ImGuiCol_NavWindowingDimBg] = ImVec4(0.025f, 0.035f, 0.039f, 0.5f);
	c[ImGuiCol_ModalWindowDimBg] = ImVec4(0.025f, 0.035f, 0.039f, 0.65f);
}

MenuStyle::MenuStyle() {
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10.f, 10.f));
}

MenuStyle::~MenuStyle() {
	ImGui::PopStyleVar();
}

DialogStyle::DialogStyle() {
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(18.f, 16.f));
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10.f, 10.f));
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10.f, 6.f));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 6.f);
}

DialogStyle::~DialogStyle() {
	ImGui::PopStyleVar(4);
}

bool TreeRow(const char *id, const char *label, const char *icon, const ImVec4 &tint,
	ImGuiTreeNodeFlags flags, float trailingWidth) {
	const float textX = ImGui::GetCursorScreenPos().x + ImGui::GetTreeNodeToLabelSpacing();
	ImGuiWindow *window = ImGui::GetCurrentWindow();
	const float right = window->WorkRect.Max.x;
	window->WorkRect.Max.x = ImMax(window->WorkRect.Min.x, right - trailingWidth);
	const bool open = ImGui::TreeNodeEx(id, flags | ImGuiTreeNodeFlags_FramePadding, "");
	window->WorkRect.Max.x = right;
	const ImVec2 min = ImGui::GetItemRectMin();
	const ImVec2 max = ImGui::GetItemRectMax();
	const float y = min.y + (max.y - min.y - ImGui::GetFontSize()) * 0.5f;
	ImDrawList *draw = ImGui::GetWindowDrawList();
	draw->PushClipRect(min, max, true);
	if (flags & ImGuiTreeNodeFlags_Selected) {
		draw->AddRectFilled(min, ImVec2(min.x + 2.f, max.y), ImGui::GetColorU32(theme::Accent));
	}
	draw->AddText(ImVec2(textX, y), ImGui::GetColorU32(tint), icon);
	draw->AddText(ImVec2(textX + 22.f, y), ImGui::GetColorU32(ImGuiCol_Text), label);
	draw->PopClipRect();
	return open;
}

bool TreeRowButton(const char *id, const char *icon, const char *tooltip, ImVec2 rowMin,
	ImVec2 rowMax, bool enabled) {
	const ImVec2 cursor = ImGui::GetCursorScreenPos();
	const float size = rowMax.y - rowMin.y;
	ImGui::SetCursorScreenPos(ImVec2(rowMax.x + ImGui::GetStyle().ItemInnerSpacing.x, rowMin.y));
	ImGui::PushID(id);
	ImGui::BeginDisabled(!enabled);
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.f, 0.f, 0.f, 0.f));
	ImGui::PushStyleColor(ImGuiCol_Text, theme::Blue);
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.f, 0.f));
	const bool clicked = ImGui::Button(icon, ImVec2(size, size));
	ImGui::PopStyleVar();
	ImGui::PopStyleColor(2);
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
		ImGui::SetTooltip("%s", tooltip);
	}
	ImGui::EndDisabled();
	ImGui::PopID();
	// Restore the already measured tree cursor. SetCursorScreenPos would
	// mark the trailing line spacing as a new layout extension at End().
	ImGui::GetCurrentWindow()->DC.CursorPos = cursor;
	return clicked;
}

bool SectionHeader(const char *label, const char *icon, const ImVec4 &tint) {
	ImGui::Dummy(ImVec2(0.f, 2.f));
	ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyleColorVec4(ImGuiCol_Button));
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.f, 6.f));
	const bool open = ImGui::CollapsingHeader(label, ImGuiTreeNodeFlags_DefaultOpen);
	ImGui::PopStyleVar();
	ImGui::PopStyleColor();
	const ImVec2 min = ImGui::GetItemRectMin();
	const ImVec2 max = ImGui::GetItemRectMax();
	// The type accent reads as a small spine without competing with the fields.
	ImGui::GetWindowDrawList()->AddRectFilled(min, ImVec2(min.x + 2.f, max.y),
		ImGui::GetColorU32(tint));
	const float iconX = max.x - 24.f;
	if (iconX > min.x + ImGui::CalcTextSize(label).x + 40.f) {
		ImGui::GetWindowDrawList()->AddText(ImVec2(iconX, min.y + 6.f), ImGui::GetColorU32(tint),
			icon);
	}
	return open;
}

} // namespace sowa::editor
