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
	ImGui::GetIO().Fonts->AddFontFromMemoryTTF(Res::Ease_include_res_Roboto_Medium_ttf_data.data(), Res::Ease_include_res_Roboto_Medium_ttf_data.size(), 14.f);
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

void GuiServer::BeginWindow(const std::string &title, uint32_t flags /*= 0*/) {
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

	ImGui::Begin(title.c_str(), nullptr, guiflags);
}
void GuiServer::EndWindow() {
	ImGui::End();
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

} // namespace Ease
