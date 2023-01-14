#include "Resource/EditorTheme/EditorTheme.hpp"
#include "Resource/ResourceLoader.hpp"
#include "Utils/YAML.hpp"
#include "imgui-docking/imgui.h"
#include "yaml-cpp/yaml.h"

namespace Sowa {
template <>
Reference<EditorTheme> ResourceLoaderImpl<EditorTheme>::Load(unsigned char *data, size_t size) {
	if (size <= 1) {
		std::cout << "Error loading editor theme." << std::endl;
		return std::make_shared<EditorTheme>();
	}
	Reference<EditorTheme> themeref = std::make_shared<EditorTheme>();
	ImGuiStyle &style = themeref->GetStyle();

	std::string text = std::string(reinterpret_cast<char *>(data), size);
	YAML::Node node = YAML::Load(text);
	if (node["Type"].as<std::string>() != "EditorTheme")
		return std::make_shared<EditorTheme>();

	// Default theme : ImGui::StyleColorsClassic();
	YAML::Node theme = node["Theme"];
	style.AntiAliasedLines = theme["AntiAliasedLines"].as<bool>(true);
	style.AntiAliasedLinesUseTex = theme["AntiAliasedLinesUseTex"].as<bool>(true);
	style.AntiAliasedFill = theme["AntiAliasedFill"].as<bool>(true);

	style.Alpha = theme["Alpha"].as<float>(1.f);
	style.DisabledAlpha = theme["DisabledAlpha"].as<float>(0.6f);
	style.WindowRounding = theme["WindowRounding"].as<float>(0.f);
	style.WindowBorderSize = theme["WindowBorderSize"].as<float>(1.f);
	style.ChildRounding = theme["ChildRounding"].as<float>(0.f);
	style.ChildBorderSize = theme["ChildBorderSize"].as<float>(1.f);
	style.PopupRounding = theme["PopupRounding"].as<float>(0.f);
	style.PopupBorderSize = theme["PopupBorderSize"].as<float>(1.f);
	style.FrameRounding = theme["FrameRounding"].as<float>(0.f);
	style.FrameBorderSize = theme["FrameBorderSize"].as<float>(0.f);
	style.IndentSpacing = theme["IndentSpacing"].as<float>(21.f);
	style.ColumnsMinSpacing = theme["ColumnsMinSpacing"].as<float>(6.f);
	style.ScrollbarSize = theme["ScrollbarSize"].as<float>(14.f);
	style.ScrollbarRounding = theme["ScrollbarRounding"].as<float>(9.f);
	style.GrabMinSize = theme["GrabMinSize"].as<float>(10.f);
	style.GrabRounding = theme["GrabRounding"].as<float>(0.f);
	style.LogSliderDeadzone = theme["LogSliderDeadzone"].as<float>(4.f);
	style.TabRounding = theme["TabRounding"].as<float>(4.f);
	style.TabBorderSize = theme["TabBorderSize"].as<float>(0.f);
	style.TabMinWidthForCloseButton = theme["TabMinWidthForCloseButton"].as<float>(0.f);
	style.MouseCursorScale = theme["MouseCursorScale"].as<float>(1.f);
	style.CurveTessellationTol = theme["CurveTessellationTol"].as<float>(1.25f);
	style.CircleTessellationMaxError = theme["CircleTessellationMaxError"].as<float>(0.3f);

	style.WindowPadding = theme["WindowPadding"].as<ImVec2>(ImVec2(8, 8));
	style.WindowMinSize = theme["WindowMinSize"].as<ImVec2>(ImVec2(32, 32));
	style.WindowTitleAlign = theme["WindowTitleAlign"].as<ImVec2>(ImVec2(0, 0.5f));
	style.FramePadding = theme["FramePadding"].as<ImVec2>(ImVec2(4, 3));
	style.ItemSpacing = theme["ItemSpacing"].as<ImVec2>(ImVec2(8, 4));
	style.ItemInnerSpacing = theme["ItemInnerSpacing"].as<ImVec2>(ImVec2(4, 4));
	style.CellPadding = theme["ItemInnerSpacing"].as<ImVec2>(ImVec2(4, 2));
	style.TouchExtraPadding = theme["ItemInnerSpacing"].as<ImVec2>(ImVec2(0, 0));
	style.ButtonTextAlign = theme["ButtonTextAlign"].as<ImVec2>(ImVec2(0.5f, 0.5f));
	style.SelectableTextAlign = theme["SelectableTextAlign"].as<ImVec2>(ImVec2(0, 0));
	style.DisplayWindowPadding = theme["DisplayWindowPadding"].as<ImVec2>(ImVec2(19, 19));
	style.DisplaySafeAreaPadding = theme["DisplaySafeAreaPadding"].as<ImVec2>(ImVec2(3, 3));

	style.Colors[ImGuiCol_Text] = theme["Text"].as<ImVec4>(ImVec4(0.9f, 0.9f, 0.9f, 1));
	style.Colors[ImGuiCol_TextDisabled] = theme["TextDisabled"].as<ImVec4>(ImVec4(0.6f, 0.6f, 0.6f, 1));
	style.Colors[ImGuiCol_WindowBg] = theme["WindowBg"].as<ImVec4>(ImVec4(0.f, 0.f, 0.f, 0.85f));
	style.Colors[ImGuiCol_ChildBg] = theme["ChildBg"].as<ImVec4>(ImVec4(0.f, 0.f, 0.f, 0.f));
	style.Colors[ImGuiCol_PopupBg] = theme["PopupBg"].as<ImVec4>(ImVec4(0.11f, 0.11f, 0.14f, 0.92f));
	style.Colors[ImGuiCol_Border] = theme["Border"].as<ImVec4>(ImVec4(0.5f, 0.5f, 0.5f, 0.5f));
	style.Colors[ImGuiCol_BorderShadow] = theme["BorderShadow"].as<ImVec4>(ImVec4(0.f, 0.f, 0.f, 0.f));
	style.Colors[ImGuiCol_FrameBg] = theme["FrameBg"].as<ImVec4>(ImVec4(0.43f, 0.43f, 0.43f, 0.39f));
	style.Colors[ImGuiCol_FrameBgHovered] = theme["FrameBgHovered"].as<ImVec4>(ImVec4(0.47f, 0.47f, 0.69f, 0.4f));
	style.Colors[ImGuiCol_FrameBgActive] = theme["FrameBgActive"].as<ImVec4>(ImVec4(0.42f, 0.41f, 0.64f, 0.69f));
	style.Colors[ImGuiCol_TitleBg] = theme["TitleBg"].as<ImVec4>(ImVec4(0.27f, 0.27f, 0.54f, 0.83f));
	style.Colors[ImGuiCol_TitleBgActive] = theme["TitleBgActive"].as<ImVec4>(ImVec4(0.32f, 0.32f, 0.63f, 0.87f));
	style.Colors[ImGuiCol_TitleBgCollapsed] = theme["TitleBgCollapsed"].as<ImVec4>(ImVec4(0.4f, 0.4f, 0.8f, 0.2f));
	style.Colors[ImGuiCol_MenuBarBg] = theme["MenuBarBg"].as<ImVec4>(ImVec4(0.4f, 0.4f, 0.55f, 0.8f));
	style.Colors[ImGuiCol_ScrollbarBg] = theme["ScrollbarBg"].as<ImVec4>(ImVec4(0.2f, 0.25f, 0.3f, 0.6f));
	style.Colors[ImGuiCol_ScrollbarGrab] = theme["ScrollbarGrab"].as<ImVec4>(ImVec4(0.4f, 0.4f, 0.8f, 0.3f));
	style.Colors[ImGuiCol_ScrollbarGrabHovered] = theme["ScrollbarGrabHovered"].as<ImVec4>(ImVec4(0.4f, 0.4f, 0.8f, 0.4f));
	style.Colors[ImGuiCol_ScrollbarGrabActive] = theme["ScrollbarGrabActive"].as<ImVec4>(ImVec4(0.41f, 0.39f, 0.8f, 0.6f));
	style.Colors[ImGuiCol_CheckMark] = theme["CheckMark"].as<ImVec4>(ImVec4(0.9f, 0.9f, 0.9f, 0.5f));
	style.Colors[ImGuiCol_SliderGrab] = theme["SliderGrab"].as<ImVec4>(ImVec4(1, 1, 1, 0.3f));
	style.Colors[ImGuiCol_SliderGrabActive] = theme["SliderGrabActive"].as<ImVec4>(ImVec4(0.41f, 0.39f, 0.8f, 0.6f));
	style.Colors[ImGuiCol_Button] = theme["Button"].as<ImVec4>(ImVec4(0.35f, 0.4f, 0.61f, 0.62f));
	style.Colors[ImGuiCol_ButtonHovered] = theme["ButtonHovered"].as<ImVec4>(ImVec4(0.4f, 0.48f, 0.71f, 0.79f));
	style.Colors[ImGuiCol_ButtonActive] = theme["ButtonActive"].as<ImVec4>(ImVec4(0.46f, 0.54f, 0.8f, 1));
	style.Colors[ImGuiCol_Header] = theme["Header"].as<ImVec4>(ImVec4(0.4f, 0.4f, 0.9f, 0.45f));
	style.Colors[ImGuiCol_HeaderHovered] = theme["HeaderHovered"].as<ImVec4>(ImVec4(0.45f, 0.45f, 0.9f, 0.8f));
	style.Colors[ImGuiCol_HeaderActive] = theme["HeaderActive"].as<ImVec4>(ImVec4(0.53f, 0.53f, 0.87f, 0.8f));
	style.Colors[ImGuiCol_Separator] = theme["Separator"].as<ImVec4>(ImVec4(0.5f, 0.5f, 0.5f, 0.6f));
	style.Colors[ImGuiCol_SeparatorHovered] = theme["SeparatorHovered"].as<ImVec4>(ImVec4(0.6f, 0.6f, 0.7f, 1));
	style.Colors[ImGuiCol_SeparatorActive] = theme["SeparatorActive"].as<ImVec4>(ImVec4(0.7f, 0.7f, 0.9f, 1));
	style.Colors[ImGuiCol_ResizeGrip] = theme["ResizeGrip"].as<ImVec4>(ImVec4(1, 1, 1, 0.1f));
	style.Colors[ImGuiCol_ResizeGripHovered] = theme["ResizeGripHovered"].as<ImVec4>(ImVec4(0.78f, 0.82f, 1, 0.6f));
	style.Colors[ImGuiCol_ResizeGripActive] = theme["ResizeGripActive"].as<ImVec4>(ImVec4(0.78f, 0.82f, 1, 0.9f));
	style.Colors[ImGuiCol_Tab] = theme["Tab"].as<ImVec4>(ImVec4(0.34f, 0.34f, 0.68f, 0.79f));
	style.Colors[ImGuiCol_TabHovered] = theme["TabHovered"].as<ImVec4>(ImVec4(0.45f, 0.45f, 0.9f, 0.8f));
	style.Colors[ImGuiCol_TabActive] = theme["TabActive"].as<ImVec4>(ImVec4(0.4f, 0.4f, 0.72f, 0.85f));
	style.Colors[ImGuiCol_TabUnfocused] = theme["TabUnfocused"].as<ImVec4>(ImVec4(0.28f, 0.28f, 0.57f, 0.82f));
	style.Colors[ImGuiCol_TabUnfocusedActive] = theme["TabUnfocusedActive"].as<ImVec4>(ImVec4(0.35f, 0.35f, 0.65f, 0.84f));
	style.Colors[ImGuiCol_DockingPreview] = theme["DockingPreview"].as<ImVec4>(ImVec4(0.4f, 0.4f, 0.9f, 0.31f));
	style.Colors[ImGuiCol_DockingEmptyBg] = theme["DockingEmptyBg"].as<ImVec4>(ImVec4(0.2f, 0.2f, 0.2f, 1));
	style.Colors[ImGuiCol_PlotLines] = theme["PlotLines"].as<ImVec4>(ImVec4(1, 1, 1, 1));
	style.Colors[ImGuiCol_PlotLinesHovered] = theme["PlotLinesHovered"].as<ImVec4>(ImVec4(0.9f, 0.7f, 0, 1));
	style.Colors[ImGuiCol_PlotHistogram] = theme["PlotHistogram"].as<ImVec4>(ImVec4(0.9f, 0.7f, 0, 1));
	style.Colors[ImGuiCol_PlotHistogramHovered] = theme["PlotHistogramHovered"].as<ImVec4>(ImVec4(1, 0.6f, 0, 1));
	style.Colors[ImGuiCol_TableHeaderBg] = theme["TableHeaderBg"].as<ImVec4>(ImVec4(0.27f, 0.27f, 0.38f, 1));
	style.Colors[ImGuiCol_TableBorderStrong] = theme["TableHeaderBg"].as<ImVec4>(ImVec4(0.31f, 0.31f, 0.45f, 1));
	style.Colors[ImGuiCol_TableBorderLight] = theme["TableBorderLight"].as<ImVec4>(ImVec4(0.26f, 0.26f, 0.28f, 1));
	style.Colors[ImGuiCol_TableRowBg] = theme["TableRowBg"].as<ImVec4>(ImVec4(0, 0, 0, 0));
	style.Colors[ImGuiCol_TableRowBgAlt] = theme["TableRowBgAlt"].as<ImVec4>(ImVec4(1, 1, 1, 0.07f));
	style.Colors[ImGuiCol_TextSelectedBg] = theme["TextSelectedBg"].as<ImVec4>(ImVec4(0, 0, 1, 0.35f));
	style.Colors[ImGuiCol_DragDropTarget] = theme["DragDropTarget"].as<ImVec4>(ImVec4(1, 1, 0, 0.9f));
	style.Colors[ImGuiCol_NavHighlight] = theme["NavHighlight"].as<ImVec4>(ImVec4(0.45f, 0.45f, 0.9f, 0.8f));
	style.Colors[ImGuiCol_NavWindowingHighlight] = theme["NavWindowingHighlight"].as<ImVec4>(ImVec4(1, 1, 1, 0.7f));
	style.Colors[ImGuiCol_NavWindowingDimBg] = theme["NavWindowingDimBg"].as<ImVec4>(ImVec4(0.8f, 0.8f, 0.8f, 0.2f));
	style.Colors[ImGuiCol_ModalWindowDimBg] = theme["ModalWindowDimBg"].as<ImVec4>(ImVec4(0.2f, 0.2f, 0.2f, 0.35f));

	return themeref;
}

EditorTheme::EditorTheme() {
}

EditorTheme::~EditorTheme() {
}

void EditorTheme::LoadFromStyle(ImGuiStyle _style) {
	style = _style;
}
} // namespace Sowa