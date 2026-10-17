#include <doctest/doctest.h>
#include <imgui.h>
#include <imgui_internal.h>

#include "editor/tilemap_palette.hpp"
#include "resource/resource_registry.hpp"

using namespace sowa;

TEST_CASE("Tilemap palette opens and resizes without ImGui layout errors") {
	struct Context {
		ImGuiContext *imgui = ImGui::CreateContext();

		~Context() { ImGui::DestroyContext(imgui); }
	} context;

	auto &io = ImGui::GetIO();
	io.IniFilename = nullptr;
	io.DisplaySize = ImVec2(1024, 768);
	// Report recoverable errors as test failures instead of aborting the suite.
	io.ConfigErrorRecoveryEnableAssert = false;
	io.ConfigErrorRecoveryEnableTooltip = false;
	int errors = 0;
	context.imgui->ErrorCallbackUserData = &errors;
	context.imgui->ErrorCallback = [](ImGuiContext *, void *data, const char *) {
		++*static_cast<int *>(data);
	};
	unsigned char *pixels;
	int width, height;
	io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
	ResourceRegistry resources;

	for (const int count : {0, 1, 3, 256}) {
		CAPTURE(count);
		Tilemap map;
		for (int i = 0; i < count; ++i) {
			map.AddScene({1}, "Prefab");
		}
		for (const float panelWidth : {320.f, 180.f, 500.f}) {
			CAPTURE(panelWidth);
			// Allow the child window to settle after opening/resizing, then scroll
			// to its final row (where the old cursor-only padding asserted).
			for (int frame = 0; frame < 3; ++frame) {
				CAPTURE(frame);
				errors = 0;
				ImGui::NewFrame();
				ImGui::SetNextWindowPos(ImVec2(0, 0));
				ImGui::SetNextWindowSize(ImVec2(panelWidth, 240));
				ImGui::Begin("Tilemap");
				CHECK_FALSE(editor::DrawTilemapPalette(map, resources, 0));
				if (frame == 1) {
					for (auto *window : context.imgui->Windows) {
						if (window->ParentWindow == ImGui::GetCurrentWindow()) {
							ImGui::SetScrollY(window, window->ScrollMax.y);
						}
					}
				}
				ImGui::End();
				ImGui::Render();
				CHECK(errors == 0);
			}
		}
	}
}
