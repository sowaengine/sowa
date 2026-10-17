#include <doctest/doctest.h>
#include <imgui.h>
#include <imgui_internal.h>

#include "editor/icons.hpp"
#include "editor/resource_inspector.hpp"
#include "editor/sprite_sheet_panel.hpp"
#include "resource/image_texture.hpp"

using namespace sowa;

namespace {
struct PanelFixture {
	ImGuiContext *context = ImGui::CreateContext();
	ResourceRegistry resources;
	Ref<SpriteSheetAnimation> library;
	editor::SpriteSheetPanel panel;
	std::string animation = "Running";
	int errors = 0;

	PanelFixture() {
		auto &io = ImGui::GetIO();
		io.IniFilename = nullptr;
		io.DisplaySize = {1400, 800};
		io.DeltaTime = 1.f / 60;
		io.ConfigErrorRecoveryEnableAssert = false;
		io.ConfigErrorRecoveryEnableTooltip = false;
		context->ErrorCallbackUserData = &errors;
		context->ErrorCallback = [](ImGuiContext *, void *data, const char *) {
			++*static_cast<int *>(data);
		};
		unsigned char *pixels;
		int width, height;
		io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
		resources.RegisterType<SpriteSheetAnimation>("SpriteSheetAnimation");
		resources.RegisterType<ImageTexture>("ImageTexture");
		auto texture = resources.Create<ImageTexture>();
		texture->Adopt(resources.Context(), {17, 1}, 128, 64);
		SpriteAnimation clip;
		clip.texture = texture->Id();
		clip.columns = 4;
		clip.rows = 2;
		clip.frames = {0, 1, 2};
		library = resources.Create<SpriteSheetAnimation>();
		library->Set(animation, clip);
	}

	~PanelFixture() { ImGui::DestroyContext(context); }

	ImGuiWindow *sequence() {
		for (auto *window : context->Windows) {
			if (std::string(window->Name).find("/Sequence_") != std::string::npos) {
				return window;
			}
		}
		return nullptr;
	}

	bool draw(const char *action = nullptr, ImVec2 size = {1100, 600}) {
		ImGui::NewFrame();
		ImGui::SetNextWindowPos({0, 0});
		ImGui::SetNextWindowSize(size);
		ImGui::Begin("Sprite Sheet");
		if (action && sequence()) {
			context->NavActivateId = sequence()->GetID(action);
			context->NavActivateDownId = context->NavActivateId;
		}
		const bool changed =
			panel.Draw(*library, animation, resources, [](ResourceID &) { return false; });
		ImGui::End();
		ImGui::Render();
		CHECK(errors == 0);
		return changed;
	}
};
} // namespace

TEST_CASE("Sprite sheet frame arrows disable at boundaries and preserve the moved selection") {
	PanelFixture f;
	f.draw();
	f.draw();
	REQUIRE(f.sequence());
	CHECK_FALSE(f.draw("up"));
	CHECK(f.library->Find("Running")->frames == std::vector<i32>{0, 1, 2});
	CHECK(f.draw("down"));
	CHECK(f.library->Find("Running")->frames == std::vector<i32>{1, 0, 2});
	f.draw();
	CHECK(f.draw("down"));
	CHECK(f.library->Find("Running")->frames == std::vector<i32>{1, 2, 0});
	f.draw();
	CHECK_FALSE(f.draw("down"));
	CHECK(f.draw("up"));
	CHECK(f.library->Find("Running")->frames == std::vector<i32>{1, 0, 2});
	CHECK(f.draw(ICON_FA_TRASH "##delete_frame"));
	CHECK(f.library->Find("Running")->frames == std::vector<i32>{1, 2});
	f.library->Find("Running")->frames.clear();
	CHECK_FALSE(f.draw("up"));
	CHECK_FALSE(f.draw("down"));
	CHECK_FALSE(f.draw(ICON_FA_TRASH "##delete_frame"));
}

TEST_CASE("Sprite sheet resource inspector offers Edit directly instead of Properties") {
	PanelFixture f;
	editor::ResourceInspector inspector;
	std::vector<editor::SceneDocumentSaveState> documents;
	for (int frame = 0; frame < 3; ++frame) {
		ImGui::NewFrame();
		ImGui::SetNextWindowPos({0, 0});
		ImGui::SetNextWindowSize({260, 500});
		ImGui::Begin("Inspector");
		ImGui::PushID("animations");
		const auto editId = ImGui::GetID("Edit");
		ImGui::PopID();
		if (frame == 2) {
			f.context->NavActivateId = editId;
			f.context->NavActivateDownId = editId;
		}
		const auto result = inspector.Draw("animations", f.library->Id(), "SpriteSheetAnimation",
			{}, f.resources, nullptr, documents, true);
		CHECK(f.context->LastItemData.ID == editId);
		CHECK(result.editRequested == (frame == 2));
		CHECK_FALSE(result.changed);
		ImGui::End();
		ImGui::Render();
		CHECK(f.errors == 0);
	}
}

TEST_CASE("Sprite animation picker selects a named animation from its dropdown") {
	PanelFixture f;
	f.library->Set("Idle", *f.library->Find("Running"));
	for (int frame = 0; frame < 5; ++frame) {
		ImGui::NewFrame();
		ImGui::SetNextWindowPos({0, 0});
		ImGui::SetNextWindowSize({300, 300});
		ImGui::Begin("Inspector");
		if (frame == 1) {
			f.context->NavActivateId = ImGui::GetID("Current Animation");
			f.context->NavActivateDownId = f.context->NavActivateId;
		}
		if (frame == 3) {
			auto *popup = ImGui::FindWindowByName("##Combo_00");
			REQUIRE(popup);
			f.context->NavActivateId = popup->GetID("Idle");
			f.context->NavActivateDownId = f.context->NavActivateId;
		}
		const bool changed =
			editor::DrawSpriteAnimationPicker("Current Animation", f.library.get(), f.animation);
		if (frame == 3) {
			CHECK(changed);
			CHECK(f.animation == "Idle");
		}
		ImGui::End();
		ImGui::Render();
	}
}
