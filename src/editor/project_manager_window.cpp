#include "editor/project_manager.hpp"

#include <algorithm>
#include <filesystem>
#include <vector>

#include "core/build_info.hpp"
#include "core/commands.hpp"
#include "core/log.hpp"
#include "core/window.hpp"
#include "editor/editor.hpp"
#include "editor/icons.hpp"
#include "editor/style.hpp"
#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>
#define GLFW_INCLUDE_NONE
#include "render/graphics_device.hpp"
#include <GLFW/glfw3.h>

#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif
#ifndef _WIN32
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>
extern char **environ;
#endif

namespace sowa::editor {
namespace {
namespace fs = std::filesystem;

bool launch(const std::string &command, const std::string &project, std::string &error) {
#ifndef _WIN32
	std::string executable;
#ifdef __APPLE__
	uint32_t size = 0;
	_NSGetExecutablePath(nullptr, &size);
	std::vector<char> path(size);
	if (_NSGetExecutablePath(path.data(), &size) != 0) {
		error = "Cannot locate editor executable.";
		return false;
	}
	executable = path.data();
#else
	std::error_code ec;
	executable = fs::read_symlink("/proc/self/exe", ec).string();
	if (ec) {
		error = ec.message();
		return false;
	}
#endif
	std::vector<char *> args{executable.data(), const_cast<char *>(command.c_str())};
	if (!project.empty()) {
		args.push_back(const_cast<char *>(project.c_str()));
	}
	args.push_back(nullptr);
	pid_t pid;
	const int result =
		posix_spawn(&pid, executable.c_str(), nullptr, nullptr, args.data(), environ);
	if (result != 0) {
		error = std::strerror(result);
		return false;
	}
	error.clear();
	return true;
#else
	error = "Project launching is not yet supported on Windows.";
	return false;
#endif
}

} // namespace

bool LaunchProjectManager(std::string &error) {
	return launch("project-manager", "", error);
}

int RunProjectManager() {
	ProjectManager manager(ProjectManager::DefaultRegistryPath());
	std::string error;
	bool loaded = manager.Load(error);
	Window window;
	auto graphics = CreateGraphicsDevice(GraphicsAPI::OpenGL);
	if (!graphics || !Window::EnsureGlfwInitialized()) {
		return 1;
	}
	graphics->ApplyWindowHints();
	if (!window.Create(1080, 720, "Project Manager") || !graphics->Init(window)) {
		return 1;
	}
	glfwSwapInterval(1);
	FileSystem files;
	Editor chrome;
	if (!chrome.Init(window, files)) {
		graphics->Shutdown();
		return 1;
	}
	std::string search, name, location;
	fs::path browseDirectory = fs::current_path();
	bool creating = false;
	std::string dialogError;
	while (!window.ShouldClose()) {
#ifndef _WIN32
		while (waitpid(-1, nullptr, WNOHANG) > 0) {
		}
#endif
		window.PollEvents();
		chrome.BeginFrame();
		const auto *viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->WorkPos);
		ImGui::SetNextWindowSize(viewport->WorkSize);
		ImGui::Begin("Project Manager", nullptr,
			ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
				ImGuiWindowFlags_NoSavedSettings);
		ImGui::TextUnformatted("Project Manager");
		ImGui::SameLine();
		ImGui::TextDisabled("Sowa Editor %s", BuildInfo::Version());
		ImGui::Spacing();
		if (ImGui::Button("Refresh", {100, 32})) {
			loaded = manager.Load(error);
		}
		ImGui::Separator();
		if (!error.empty()) {
			ImGui::TextWrapped("%s", error.c_str());
		}
		ImGui::BeginDisabled(!loaded);
		if (ImGui::Button(ICON_FA_PLUS "  New project", {150, 32})) {
			creating = true;
			name.clear();
			location.clear();
			dialogError.clear();
			ImGui::OpenPopup("Project details");
		}
		ImGui::SameLine();
		if (ImGui::Button(ICON_FA_FOLDER_OPEN "  Add existing", {150, 32})) {
			creating = false;
			location.clear();
			dialogError.clear();
			ImGui::OpenPopup("Project details");
		}
		ImGui::SameLine();
		ImGui::SetNextItemWidth(-1);
		ImGui::InputTextWithHint("##search", "Search projects by name or path", &search);
		ImGui::Spacing();
		if (manager.Projects().empty()) {
			ImGui::TextDisabled("No projects registered.");
		}
		std::string openPath, removePath;
		if (ImGui::BeginTable("projects", 4,
				ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_ScrollY,
				{0, ImGui::GetContentRegionAvail().y - 30})) {
			ImGui::TableSetupColumn("Project", ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableSetupColumn("Editor version", ImGuiTableColumnFlags_WidthFixed, 110);
			ImGui::TableSetupColumn("Last opened", ImGuiTableColumnFlags_WidthFixed, 160);
			ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthFixed, 165);
			ImGui::TableHeadersRow();
			for (const auto &project : manager.Projects()) {
				if (!MatchesCommand(project.name + " " + project.path, search)) {
					continue;
				}
				ImGui::PushID(project.path.c_str());
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::TextUnformatted(project.name.c_str());
				ImGui::TextDisabled("%s", project.path.c_str());
				std::error_code ec;
				const bool exists =
					fs::is_regular_file(fs::path(project.path) / "project.sowa", ec);
				if (!exists) {
					ImGui::TextColored(theme::Rose, "Project not found at this location");
				}
				ImGui::TableNextColumn();
				ImGui::TextUnformatted(
					project.editorVersion.empty() ? "Current" : project.editorVersion.c_str());
				ImGui::TableNextColumn();
				ImGui::TextUnformatted(
					project.lastOpened.empty() ? "Never" : project.lastOpened.c_str());
				ImGui::TableNextColumn();
				ImGui::BeginDisabled(!exists);
				if (ImGui::Button("Open")) {
					openPath = project.path;
				}
				ImGui::EndDisabled();
				ImGui::SameLine();
				if (ImGui::Button("Remove")) {
					removePath = project.path;
				}
				if (ImGui::IsItemHovered()) {
					ImGui::SetTooltip("Remove from this list. Project files stay on disk.");
				}
				ImGui::PopID();
			}
			ImGui::EndTable();
		}
		if (!removePath.empty()) {
			manager.Remove(removePath, error);
		}
		if (!openPath.empty()) {
			ManagedProject checked;
			if (ProjectManager::Inspect(openPath, checked, error) &&
				launch("edit", openPath, error)) {
				manager.MarkOpened(openPath, BuildInfo::Version(), error);
			}
		}
		ImGui::EndDisabled();
		ImGui::SetNextWindowSize({640, 320}, ImGuiCond_Appearing);
		if (const DialogStyle style;
			ImGui::BeginPopupModal("Project details", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
			ImGui::TextUnformatted(creating ? "Create a new project" : "Add an existing project");
			if (creating) {
				ImGui::InputText("Project name", &name);
			}
			ImGui::InputText(creating ? "New folder" : "Project folder or file", &location);
			if (ImGui::Button("Browse folders...")) {
				ImGui::OpenPopup("Choose folder");
			}
			ImGui::TextWrapped(
				creating ? "Choose a parent folder, then enter a new subfolder in the path above."
						 : "Select a folder containing project.sowa, or enter the path to "
						   "project.sowa.");
			if (!dialogError.empty()) {
				ImGui::TextWrapped("%s", dialogError.c_str());
			}
			if (ImGui::Button(creating ? "Create project" : "Add project")) {
				if (creating ? manager.Create(location, name, dialogError)
							 : manager.Add(location, dialogError)) {
					ImGui::CloseCurrentPopup();
				}
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancel")) {
				ImGui::CloseCurrentPopup();
			}
			ImGui::SetNextWindowSize({600, 440}, ImGuiCond_Appearing);
			if (ImGui::BeginPopupModal("Choose folder", nullptr)) {
				ImGui::TextWrapped("%s", browseDirectory.string().c_str());
				if (ImGui::Button("Up") && browseDirectory.has_parent_path()) {
					browseDirectory = browseDirectory.parent_path();
				}
				if (ImGui::BeginChild("folders", {0, -48})) {
					std::error_code ec;
					std::vector<fs::path> directories;
					fs::directory_iterator it(browseDirectory,
						fs::directory_options::skip_permission_denied, ec);
					for (; !ec && it != fs::directory_iterator(); it.increment(ec)) {
						std::error_code typeError;
						if (it->is_directory(typeError)) {
							directories.push_back(it->path());
						}
					}
					std::sort(directories.begin(), directories.end());
					for (const auto &directory : directories) {
						if (ImGui::Selectable(directory.filename().string().c_str())) {
							browseDirectory = directory;
						}
					}
					if (ec) {
						ImGui::TextWrapped("%s", ec.message().c_str());
					}
				}
				ImGui::EndChild();
				if (ImGui::Button("Use this folder")) {
					location = browseDirectory.string();
					ImGui::CloseCurrentPopup();
				}
				ImGui::SameLine();
				if (ImGui::Button("Cancel")) {
					ImGui::CloseCurrentPopup();
				}
				ImGui::EndPopup();
			}
			ImGui::EndPopup();
		}
		ImGui::End();
		graphics->Clear({0.08f, 0.09f, 0.12f, 1});
		chrome.EndFrame(*graphics, window);
		window.SwapBuffers();
	}
	chrome.Shutdown();
	graphics->Shutdown();
	return 0;
}
} // namespace sowa::editor
