#include "core/application.hpp"
#include "core/arg_parser.hpp"
#include "core/build_info.hpp"
#include "core/log.hpp"

#include <string>

#if SOWA_EDITOR
#include <cstdio>

#include "build/build_runner.hpp"
#include "editor/project_manager.hpp"
#endif

#if defined(__EMSCRIPTEN__)
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#endif

namespace {

#if SOWA_EDITOR
int runBuildCommand(const std::string &projectPath, const sowa::BuildCommand &command) {
	if (command.list) {
		std::printf("%s", sowa::build::DescribeBuildProject(projectPath).c_str());
		return 0;
	}
	sowa::build::BuildRequest request;
	request.projectRoot = projectPath;
	request.targets = command.targets;
	request.all = command.all;
	request.paramOverrides = command.params;
	request.templatesDirectory = command.templatesDirectory;
	const sowa::build::BuildReport report = sowa::build::RunBuild(request);
	if (!report.ok) {
		sowa::Log::Error("{}", report.error);
		return 1;
	}
	return 0;
}
#endif

void logVersion() {
	sowa::Log::Info("Sowa Engine {} ({}@{}{})", sowa::BuildInfo::Version(),
		sowa::BuildInfo::GitHash(), sowa::BuildInfo::GitBranch(),
		sowa::BuildInfo::IsDirty() ? "-dirty" : "");
}

// Mounts the project as res://, loads its settings and opens the window.
// Returns false on failure (logged).
bool startApplication(sowa::Application &app, sowa::AppMode mode, const std::string &projectPath,
	bool projectIsZip) {
	app.SetEditorEnabled(mode == sowa::AppMode::Edit);
	if (projectIsZip) {
		const auto error = app.GetFileSystem().MountZip("res", projectPath);
		if (error != sowa::FsError::Ok) {
			sowa::Log::Error("Failed to mount ZIP '{}': {}", projectPath,
				sowa::FsErrorToString(error));
			return false;
		}
	} else {
		app.GetFileSystem().MountFolder("res", projectPath);
	}

	if (!app.GetProjectSettings().Load(app.GetFileSystem())) {
		return false;
	}

	const auto &settings = app.GetProjectSettings();
	std::string title =
		settings.application.name.empty() ? std::string("Sowa Engine") : settings.application.name;
	if (mode == sowa::AppMode::Edit) {
		title += " [Editor]";
	}

	if (!app.Init(settings.rendering.window.width, settings.rendering.window.height, title)) {
		sowa::Log::Error("Failed to initialize application");
		return false;
	}
#if !defined(__EMSCRIPTEN__) && !defined(__APPLE__)
	// Web exports use the icon as the page favicon instead (see build/).
	if (!settings.application.icon.empty()) {
		const auto icon = app.GetFileSystem().Load(settings.application.icon);
		if (icon && icon.value) {
			app.GetWindow().SetIcon(icon.value->Data(), icon.value->Size());
		} else {
			sowa::Log::Warn("Window icon '{}': {}", settings.application.icon,
				sowa::FsErrorToString(icon.error));
		}
	}
#endif

#if SOWA_EDITOR
	if (mode == sowa::AppMode::Edit) {
		sowa::editor::ProjectManager manager(sowa::editor::ProjectManager::DefaultRegistryPath());
		std::string error;
		if (!manager.Load(error) || !manager.Add(projectPath, error)) {
			sowa::Log::Warn("Project Manager: {}", error);
		}
	}
#endif

	sowa::Log::Info("Loaded project '{}' by '{}'", settings.application.name,
		settings.application.author);
	return true;
}

#if defined(__EMSCRIPTEN__)

// Web export templates run game.zip from the page's directory. The browser
// owns the frame loop, so the application lives for the page's lifetime.
sowa::Scope<sowa::Application> webApp;

// The canvas fills the page. A page without layout yet reports 0x0; the
// project's window size stays until the next resize.
void fitCanvasToPage() {
	const int width = EM_ASM_INT({ return window.innerWidth; });
	const int height = EM_ASM_INT({ return window.innerHeight; });
	if (width > 0 && height > 0) {
		webApp->GetWindow().SetSize(width, height);
	}
}

// Lets the page hide its loading screen, or show that startup failed.
void notifyPageGameStarted(bool ok) {
	EM_ASM(
		{
			if (Module.onGameStart) {
				Module.onGameStart(!!$0);
			}
		},
		ok);
}

void onArchiveLoaded(const char *file) {
	webApp = sowa::MakeScope<sowa::Application>();
	if (!startApplication(*webApp, sowa::AppMode::Run, file, true)) {
		webApp.reset();
		notifyPageGameStarted(false);
		return;
	}
	fitCanvasToPage();
	emscripten_set_resize_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, nullptr, false,
		[](int, const EmscriptenUiEvent *, void *) {
			fitCanvasToPage();
			return false;
		});
	// Hidden tabs get no frames; don't count that pause as one huge delta.
	emscripten_set_visibilitychange_callback(nullptr, false,
		[](int, const EmscriptenVisibilityChangeEvent *event, void *) {
			if (!event->hidden) {
				webApp->ResetFrameClock();
			}
			return false;
		});
	emscripten_set_main_loop([] { webApp->Update(); }, 0, false);
	notifyPageGameStarted(true);
}

#endif

} // namespace

int main(int argc, char *argv[]) {
	sowa::ArgParser argParser(SOWA_EDITOR);
	switch (argParser.Parse(argc, argv)) {
	case sowa::ArgParseResult::ExitSuccess:
		return 0;
	case sowa::ArgParseResult::ExitFailure:
		return 1;
	case sowa::ArgParseResult::Continue:
		break;
	}

#if defined(__EMSCRIPTEN__)
	logVersion();
	emscripten_async_wget("game.zip", "/game.zip", onArchiveLoaded, [](const char *) {
		sowa::Log::Error("Failed to download game.zip");
		notifyPageGameStarted(false);
	});
	return 0;
#else
#if SOWA_EDITOR
	if (argParser.Mode() == sowa::AppMode::ProjectManager) {
		return sowa::editor::RunProjectManager();
	}
	if (argParser.Mode() == sowa::AppMode::Build) {
		return runBuildCommand(argParser.ProjectDirectory(), argParser.Build());
	}
#endif

	logVersion();
	sowa::Log::Info("Project path: {}", argParser.ProjectDirectory());

	sowa::Application app;
	sowa::Application::DebugAdapterOptions debugOptions;
	if (argParser.Debug().port) {
		debugOptions.port = *argParser.Debug().port;
	}
	debugOptions.waitForDebugger = argParser.Debug().waitForDebugger;
	app.SetDebugAdapterOptions(debugOptions);
	if (!startApplication(app, argParser.Mode(), argParser.ProjectDirectory(),
			argParser.ProjectIsZip())) {
		return 1;
	}

	while (!app.ShouldClose()) {
		app.Update();
	}

	app.Shutdown();

	sowa::Log::Info("Sowa Engine shutting down.");
	return 0;
#endif
}
