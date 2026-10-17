#include <doctest/doctest.h>

#include "editor/style.hpp"
#include "imgui.h"
#include "imgui_internal.h"

using namespace sowa;

TEST_CASE("Styled tree rows preserve native click targeting and nested tree state") {
	ImGui::CreateContext();

	struct Cleanup {
		~Cleanup() {
			ImGui::DestroyContext();
		}
	} cleanup;

	auto &io = ImGui::GetIO();
	io.IniFilename = nullptr;
	io.DisplaySize = {480.f, 360.f};
	io.DeltaTime = 1.f / 60.f;
	unsigned char *pixels;
	int width, height;
	io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
	editor::ApplyStyle(ImGui::GetStyle());
	ImVec2 childCenter;
	bool clicked = false;
	const auto frame = [&] {
		ImGui::NewFrame();
		ImGui::SetNextWindowPos({0.f, 0.f});
		ImGui::SetNextWindowSize({300.f, 300.f});
		ImGui::Begin("Tree test", nullptr, ImGuiWindowFlags_NoTitleBar);
		const auto rootId = ImGui::GetID("root");
		ImGui::SetNextItemOpen(true, ImGuiCond_Once);
		if (editor::TreeRow("root",
				"Scene",
				"+",
				editor::theme::Blue,
				ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_OpenOnArrow)) {
			const auto childId = ImGui::GetID("child");
			editor::TreeRow("child",
				"Sprite",
				"*",
				editor::theme::Accent,
				ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen |
					ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_Selected);
			CHECK(ImGui::GetItemID() == childId);
			CHECK(ImGui::GetItemRectSize().y >= ImGui::GetFrameHeight());
			const auto min = ImGui::GetItemRectMin(), max = ImGui::GetItemRectMax();
			childCenter = {(min.x + max.x) * 0.5f, (min.y + max.y) * 0.5f};
			clicked |= ImGui::IsItemClicked();
			ImGui::TreePop();
		}
		CHECK(ImGui::GetID("root") == rootId);
		ImGui::End();
		ImGui::Render();
	};
	frame();
	frame();
	io.AddMousePosEvent(childCenter.x, childCenter.y);
	frame();
	io.AddMouseButtonEvent(ImGuiMouseButton_Left, true);
	frame();
	CHECK(clicked);
	io.AddMouseButtonEvent(ImGuiMouseButton_Left, false);
	frame();
}

TEST_CASE("Hierarchy trailing scene button does not select or expand the entity row") {
	ImGui::CreateContext();

	struct Cleanup {
		~Cleanup() {
			ImGui::DestroyContext();
		}
	} cleanup;

	auto &io = ImGui::GetIO();
	io.IniFilename = nullptr;
	io.DisplaySize = {400, 240};
	io.DeltaTime = 1.f / 60.f;
	unsigned char *pixels;
	int width, height;
	io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
	editor::ApplyStyle(ImGui::GetStyle());
	ImVec2 buttonCenter;
	bool selected = false, pressed = false;
	const auto frame = [&] {
		ImGui::NewFrame();
		ImGui::SetNextWindowPos({0, 0});
		ImGui::SetNextWindowSize({300, 200});
		ImGui::Begin("Scene icon test", nullptr, ImGuiWindowFlags_NoTitleBar);
		const float reserve = ImGui::GetFrameHeight() + ImGui::GetStyle().ItemInnerSpacing.x;
		const bool open = editor::TreeRow("instance", "An instance with a very long name", "*",
			editor::theme::Blue, ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_OpenOnArrow,
			reserve);
		CHECK_FALSE(open);
		selected |= ImGui::IsItemClicked();
		const auto min = ImGui::GetItemRectMin(), max = ImGui::GetItemRectMax();
		const auto cursor = ImGui::GetCursorScreenPos();
		pressed |= editor::TreeRowButton("instance-source", ">", "Open scene", min, max);
		CHECK(ImGui::GetItemRectMin().x > max.x);
		const auto buttonMin = ImGui::GetItemRectMin(), buttonMax = ImGui::GetItemRectMax();
		buttonCenter = {(buttonMin.x + buttonMax.x) / 2, (buttonMin.y + buttonMax.y) / 2};
		CHECK(ImGui::GetCursorScreenPos().y == cursor.y);
		ImGui::End();
		ImGui::Render();
	};
	frame();
	frame();
	io.AddMousePosEvent(buttonCenter.x, buttonCenter.y);
	frame();
	io.AddMouseButtonEvent(ImGuiMouseButton_Left, true);
	frame();
	io.AddMouseButtonEvent(ImGuiMouseButton_Left, false);
	frame();
	CHECK(pressed);
	CHECK_FALSE(selected);
}
