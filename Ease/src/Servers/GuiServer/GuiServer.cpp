#include "Servers/GuiServer/GuiServer.hpp"

#include "imgui-docking/backends/imgui_impl_glfw.h"
#include "imgui-docking/backends/imgui_impl_opengl3.h"
#include "imgui-docking/imgui.h"

#include <GLFW/glfw3.h>

#include "res/Roboto-Medium.ttf.h"

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
	ImGui::GetIO().Fonts->AddFontFromMemoryTTF(Res::roboto_medium_data.data(), Res::roboto_medium_data.size(), 14.f);
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

} // namespace Ease
