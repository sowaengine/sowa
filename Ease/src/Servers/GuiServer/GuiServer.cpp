#include "Servers/GuiServer/GuiServer.hpp"

#include "imgui-docking/backends/imgui_impl_glfw.h"
#include "imgui-docking/backends/imgui_impl_opengl3.h"
#include "imgui-docking/imgui.h"

#include <GLFW/glfw3.h>

#include "res/Roboto-Medium.ttf.res.hpp"

#include "Core/Application.hpp"
#include "Resource/EditorTheme/EditorTheme.hpp"
#include "Resource/ResourceLoader.hpp"

namespace Ease {
GuiServer::GuiServer(Application *app, EngineContext &ctx) : _Application(app), _Context(ctx) {
}
GuiServer::~GuiServer() {
}

void GuiServer::InitGui(GLFWwindow *window) {
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	ImGui::GetIO().Fonts->AddFontFromMemoryTTF(Res::include_res_Roboto_Medium_ttf_data.data(), Res::include_res_Roboto_Medium_ttf_data.size(), 16.f);
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 150");

	Reference<Ease::EditorTheme> theme = Ease::ResourceLoader::get_singleton().LoadResource<Ease::EditorTheme>("abs://res/theme.escfg");
	ImGui::GetStyle() = theme->GetStyle();
}

void GuiServer::BeginGui() {
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
}

void GuiServer::EndGui() {
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

bool GuiServer::BeginWindow(const std::string &title, uint32_t flags /*= 0*/) {
	int guiflags = 0;
	if (flags & WindowFlags_NoResize)
		guiflags |= ImGuiWindowFlags_NoResize;
	if (flags & WindowFlags_NoMove)
		guiflags |= ImGuiWindowFlags_NoMove;
	if (flags & WindowFlags_NoBringToFrontOnFocus)
		guiflags |= ImGuiWindowFlags_NoBringToFrontOnFocus;
	if (flags & WindowFlags_NoNavFocus)
		guiflags |= ImGuiWindowFlags_NoNavFocus;
	if (flags & WindowFlags_NoDocking)
		guiflags |= ImGuiWindowFlags_NoDocking;
	if (flags & WindowFlags_NoTitleBar)
		guiflags |= ImGuiWindowFlags_NoTitleBar;
	if (flags & WindowFlags_NoCollapse)
		guiflags |= ImGuiWindowFlags_NoCollapse;
	if (flags & WindowFlags_MenuBar)
		guiflags |= ImGuiWindowFlags_MenuBar;
	if (flags & WindowFlags_NoBackground)
		guiflags |= ImGuiWindowFlags_NoBackground;

	for (auto &[panelTitle, panel] : _panelViews) {
		if (panelTitle == title) {
			if (!panel.visible) {
				return false;
			}
		}
	}
	_panelStack += 1;
	return ImGui::Begin(title.c_str(), nullptr, guiflags);
}
void GuiServer::EndWindow() {
	if (_panelStack >= 1) {
		_panelStack--;
		ImGui::End();
	}
}

void GuiServer::Text(const std::string &text) {
	ImGui::Text("%s", text.c_str());
}

bool GuiServer::Button(const std::string &label, int width /*= 0*/, int height /*= 0*/) {
	return ImGui::Button(label.c_str(), ImVec2(width, height));
}

bool GuiServer::DragFloat(const std::string &label, float &f) {
	return ImGui::DragFloat(label.c_str(), &f);
}

bool GuiServer::Checkbox(const std::string &label, bool &value) {
	return ImGui::Checkbox(label.c_str(), &value);
}

void GuiServer::Separator() {
	ImGui::Separator();
}

void GuiServer::SetNextWindowPos(int x, int y) {
	ImGui::SetNextWindowPos(ImVec2(x, y), ImGuiCond_Always);
}

void GuiServer::SetNextWindowSize(int width, int height) {
	ImGui::SetNextWindowSize(ImVec2(width, height), ImGuiCond_Always);
}

void GuiServer::PushStyleVar(StyleVar var, float value) {
	ImGuiStyleVar styleVar = 0;
	if (var == StyleVar::WindowRounding) {
		styleVar = ImGuiStyleVar_WindowRounding;
	} else {
		Debug::Error("GuiServer: Invalid style var '{}' with type 'float'", (int)var);
	}

	ImGui::PushStyleVar(styleVar, value);
}
void GuiServer::PushStyleVar(StyleVar var, Vec2 value) {
	ImGuiStyleVar styleVar = 0;
	if (var == StyleVar::WindowPadding) {
		styleVar = ImGuiStyleVar_WindowPadding;
	} else {
		Debug::Error("GuiServer: Invalid style var '{}' with type 'Vec2'", (int)var);
	}

	ImGui::PushStyleVar(styleVar, ImVec2(value.x, value.y));
}
void GuiServer::PopStyleVar(int count) {
	ImGui::PopStyleVar(count);
}

void GuiServer::SetupDockspace() {
	ImGui::DockSpace(ImGui::GetID("Dockspace"), ImVec2(0.f, 0.f), ImGuiDockNodeFlags_PassthruCentralNode);
}

bool GuiServer::BeginMainMenuBar() {
	return ImGui::BeginMainMenuBar();
}

void GuiServer::EndMainMenuBar() {
	ImGui::EndMainMenuBar();
}

bool GuiServer::BeginMenu(const std::string &label) {
	return ImGui::BeginMenu(label.c_str());
}

void GuiServer::EndMenu() {
	ImGui::EndMenu();
}

bool GuiServer::MenuItem(const std::string &label, const std::string &shortcut /*= {""}*/) {
	return ImGui::MenuItem(label.c_str(), shortcut.c_str());
}

bool GuiServer::BeginFooter(const std::string &label) {
	const ImGuiViewport *viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(
		ImVec2(viewport->Pos.x,
			   viewport->Pos.y + viewport->Size.y - ImGui::GetFrameHeight()));
	ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, ImGui::GetFrameHeight()));
	PushStyleVar(Ease::StyleVar::WindowRounding, 0.f);
	// todo add Gui::PushStyleColor
	ImGui::PushStyleColor(ImGuiCol_MenuBarBg, ImVec4(0.17f, 0.50f, 0.70f, 1.f));	 // (45, 128, 178)
	ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.20f, 0.55f, 0.75f, 1.f)); // (51, 140, 191)

	bool opened = false;
	opened = ImGui::Begin(label.c_str(), nullptr,
						  ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
	ImGui::PopStyleVar();
	if (!ImGui::BeginMenuBar()) {
		return false;
	}
	return opened;
}

void GuiServer::EndFooter() {
	ImGui::EndMenuBar();
	ImGui::End();
	ImGui::PopStyleColor(2);
}

void GuiServer::RegisterPanel(const std::string &windowTitle, const std::string &header, bool visibleByDefault /*= true*/) {
	_panelViews[windowTitle] = PanelData();
	_panelViews[windowTitle].header = header;
	_panelViews[windowTitle].visible = visibleByDefault;
}

void GuiServer::DrawViewbar() {
	for (auto &[title, panel] : _panelViews) {
		ImGui::Checkbox(title.c_str(), &panel.visible);
	}
}

bool GuiServer::BeginCheckerList(const std::string &id) {
	return ImGui::BeginTable(id.c_str(), 1, ImGuiTableFlags_BordersV | ImGuiTableFlags_BordersOuterH | ImGuiTableFlags_RowBg | ImGuiTableFlags_NoBordersInBody);
}

void GuiServer::CheckerListNextItem() {
	ImGui::TableNextRow();
	ImGui::TableNextColumn();
}

void GuiServer::EndCheckerList() {
	ImGui::EndTable();
}

void GuiServer::ShowDemoWindow() {
	ImGui::ShowDemoWindow();
}

bool GuiServer::BeginTree(const std::string &label) {
	return ImGui::TreeNodeEx(label.c_str(), ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth);
}
void GuiServer::EndTree() {
	ImGui::TreePop();
}

void GuiServer::PushID(const std::string &id) {
	ImGui::PushID(id.c_str());
}
void GuiServer::PopID() {
	ImGui::PopID();
}
} // namespace Ease
