#include <doctest/doctest.h>
#include <imgui.h>
#include <imgui_internal.h>

#include "core/filesystem/file_system.hpp"
#include "core/log.hpp"
#include "ecs/components/sprite.hpp"
#include "ecs/world.hpp"
#include "editor/resource_inspector.hpp"
#include "editor/scene_commands.hpp"
#include "particles/particles2d.hpp"
#include "render/stub_graphics_device.hpp"
#include "resource/audio_clip.hpp"
#include "resource/image_texture.hpp"
#include "scene/scene_serializer.hpp"

using namespace sowa;

namespace {
struct TestMaterial : Resource {
	ResourceID texture;
	float strength = 1;
	std::string path;
	int reloads = 0;

	void Reload(const ResourceContext &) override {
		++reloads;
	}
};

struct Fixture {
	World world;
	ResourceRegistry resources;
	Scene scene = CreateScene(world, "Effects");
	std::vector<editor::SceneDocumentSaveState> documents;

	Fixture() {
		resources.RegisterType<ImageTexture>("ImageTexture").Field("path", &ImageTexture::path);
		resources.RegisterType<AudioClip>("AudioClip").Field("path", &AudioClip::path);
		resources.RegisterType<TestMaterial>("TestMaterial")
			.ResourceField("texture", &TestMaterial::texture, "ImageTexture")
			.Field("strength", &TestMaterial::strength)
			.Field("path", &TestMaterial::path);
		editor::UpdateSceneDocumentStates(documents, {scene});
	}
};
} // namespace

TEST_CASE(
	"Resource inspector: new resources belong to the scene and choices match the field type") {
	Fixture fixture;
	auto texture = editor::CreateSceneResource(fixture.scene, fixture.resources, "ImageTexture");
	auto clip = editor::CreateSceneResource(fixture.scene, fixture.resources, "AudioClip");
	REQUIRE(texture.IsValid());
	REQUIRE(fixture.scene.Owns(texture));
	CHECK(fixture.resources.OwnerCount(texture) == 1);
	CHECK(editor::SceneResourcesOfType(fixture.scene, fixture.resources, "ImageTexture") ==
		  std::vector<ResourceID>{texture});
	CHECK(editor::SceneResourcesOfType(fixture.scene, fixture.resources, "AudioClip") ==
		  std::vector<ResourceID>{clip});
	CHECK(editor::SceneResourcesOfType(fixture.scene, fixture.resources, "").size() == 2);
	CHECK(fixture.resources.TypeNames() ==
		  std::vector<std::string>{"AudioClip", "ImageTexture", "TestMaterial"});

	auto entity = fixture.scene.CreateEntity();
	entity.Add<Particles2D>();
	const auto type = fixture.world.FindComponent("Particles2D");
	CHECK(entity.GetField(type, "texture").Info()->resourceType == "ImageTexture");
	CHECK(fixture.resources.FieldsOf("TestMaterial").front().resourceType == "ImageTexture");
	fixture.resources.Get<ImageTexture>(texture)->path = "res://textures/fire.png";
	CHECK(editor::ResourceLabel(fixture.resources, texture) == "fire.png");
	CHECK(editor::ResourceLabel(fixture.resources, {}) == "Empty");
}

TEST_CASE("Resource inspector: creating and assigning a resource is one undoable scene edit") {
	Fixture fixture;
	// Use the scene root itself: creating its first resource can move ECS storage.
	auto entity = fixture.scene.Root();
	entity.Add<Particles2D>();
	const auto type = fixture.world.FindComponent("Particles2D");
	const auto *field = entity.GetField(type, "texture").Info();
	const auto texture =
		editor::CreateSceneResource(fixture.scene, fixture.resources, "ImageTexture");
	entity.GetField(type, "texture").Set(texture);
	REQUIRE(editor::RecordResourceAssignment(fixture.documents, fixture.scene, entity, type, *field,
		fixture.resources, {}, texture, true));
	REQUIRE(editor::UndoSceneDocument(fixture.documents, fixture.scene));
	CHECK_FALSE(entity.Get<Particles2D>()->texture.IsValid());
	CHECK_FALSE(fixture.scene.Owns(texture));
	CHECK(fixture.resources.Has(texture)); // retained only for redo
	REQUIRE(editor::RedoSceneDocument(fixture.documents, fixture.scene));
	CHECK(entity.Get<Particles2D>()->texture == texture);
	CHECK(fixture.scene.Owns(texture));

	auto archive = Archive::Save();
	REQUIRE(SaveScene(archive, fixture.scene, fixture.resources));
	World restoredWorld;
	ResourceRegistry restoredResources;
	restoredResources.RegisterType<ImageTexture>("ImageTexture").Field("path", &ImageTexture::path);
	auto input = Archive::Load(archive.ToString());
	auto restored = LoadScene(input, restoredWorld, restoredResources);
	REQUIRE(restored.IsValid());
	CHECK(restored.Owns(texture));
	CHECK(restored.Root().Get<Particles2D>()->texture == texture);

	REQUIRE(editor::UndoSceneDocument(fixture.documents, fixture.scene));
	fixture.documents.front().history.Clear();
	CHECK_FALSE(fixture.resources.Has(texture));
}

TEST_CASE(
	"Resource inspector: editing a shared resource reloads it and marks owning scenes dirty") {
	Fixture fixture;
	auto id = editor::CreateSceneResource(fixture.scene, fixture.resources, "TestMaterial");
	auto resource = fixture.resources.Get<TestMaterial>(id);
	auto other = CreateScene(fixture.world, "Other");
	REQUIRE(other.Own(fixture.resources, id));
	editor::UpdateSceneDocumentStates(fixture.documents, {fixture.scene, other});
	for (auto &document : fixture.documents) {
		document.history.MarkSaved();
	}
	const auto before = editor::SaveResource(*resource);
	resource->strength = 5;
	REQUIRE(editor::RecordResourceEdit(fixture.documents, fixture.scene, fixture.resources, id,
		before, editor::SaveResource(*resource)));
	CHECK(editor::IsSceneDocumentUnsaved(fixture.documents, fixture.scene));
	CHECK(editor::IsSceneDocumentUnsaved(fixture.documents, other));
	REQUIRE(editor::UndoSceneDocument(fixture.documents, fixture.scene));
	CHECK(resource->strength == 1);
	CHECK(resource->reloads == 1);
	REQUIRE(editor::RedoSceneDocument(fixture.documents, fixture.scene));
	CHECK(resource->strength == 5);
	CHECK(resource->reloads == 2);
}

TEST_CASE("Resource inspector: nested resources save and undo with their parent") {
	Fixture fixture;
	auto materialId = editor::CreateSceneResource(fixture.scene, fixture.resources, "TestMaterial");
	auto material = fixture.resources.Get<TestMaterial>(materialId);
	const auto before = editor::SaveResource(*material);
	auto texture = editor::CreateSceneResource(fixture.scene, fixture.resources, "ImageTexture");
	material->texture = texture;
	REQUIRE(editor::RecordResourceEdit(fixture.documents, fixture.scene, fixture.resources,
		materialId, before, editor::SaveResource(*material), texture));
	REQUIRE(editor::UndoSceneDocument(fixture.documents, fixture.scene));
	CHECK_FALSE(material->texture.IsValid());
	CHECK_FALSE(fixture.scene.Owns(texture));
	REQUIRE(editor::RedoSceneDocument(fixture.documents, fixture.scene));
	CHECK(material->texture == texture);
	CHECK(fixture.scene.Owns(texture));
}

TEST_CASE("Resource inspector: long paths and expanded properties fit a narrow panel") {
	Fixture fixture;
	auto id = editor::CreateSceneResource(fixture.scene, fixture.resources, "TestMaterial");
	fixture.resources.Get<TestMaterial>(id)->path =
		"res://textures/a_very_long_filename_that_must_not_expand_the_inspector_panel.png";
	ImGui::CreateContext();
	auto &io = ImGui::GetIO();
	io.IniFilename = nullptr;
	io.DisplaySize = {640, 720};
	io.DeltaTime = 1.f / 60;
	unsigned char *pixels;
	int width, height;
	io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
	editor::ResourceInspector inspector;
	for (int frame = 0; frame < 4; ++frame) {
		ImGui::NewFrame();
		ImGui::SetNextWindowPos({0, 0});
		ImGui::SetNextWindowSize({240, 650});
		ImGui::Begin("Inspector", nullptr, ImGuiWindowFlags_HorizontalScrollbar);
		ImGui::PushID("material");
		ImGui::GetStateStorage()->SetBool(ImGui::GetID("###Properties"), true);
		ImGui::PopID();
		const float left = ImGui::GetCursorScreenPos().x;
		inspector.Draw("material", id, "TestMaterial", fixture.scene, fixture.resources, nullptr,
			fixture.documents, true);
		CHECK(ImGui::GetItemRectMin().x == doctest::Approx(left));
		if (frame > 0) {
			CHECK(ImGui::GetScrollMaxX() == 0);
		}
		ImGui::End();
		ImGui::Render();
	}
	ImGui::DestroyContext();
}

TEST_CASE("Resource inspector: shared nested resources are saved by every owning scene") {
	Fixture fixture;
	const auto materialId =
		editor::CreateSceneResource(fixture.scene, fixture.resources, "TestMaterial");
	auto material = fixture.resources.Get<TestMaterial>(materialId);
	auto other = CreateScene(fixture.world, "Other");
	REQUIRE(other.Own(fixture.resources, materialId));
	editor::UpdateSceneDocumentStates(fixture.documents, {fixture.scene, other});
	const auto before = editor::SaveResource(*material);
	const auto texture =
		editor::CreateSceneResource(fixture.scene, fixture.resources, "ImageTexture");
	material->texture = texture;
	REQUIRE(editor::RecordResourceEdit(fixture.documents, fixture.scene, fixture.resources,
		materialId, before, editor::SaveResource(*material), texture));
	CHECK(other.Owns(texture));
	REQUIRE(editor::UndoSceneDocument(fixture.documents, other));
	CHECK_FALSE(other.Owns(texture));
	REQUIRE(editor::RedoSceneDocument(fixture.documents, other));
	CHECK(other.Owns(texture));
	CHECK(material->texture == texture);
}

TEST_CASE("Resource inspector: texture thumbnails match viewport UVs for both flip settings") {
	struct PreviewDevice : StubGraphicsDevice {
		u64 GetNativeTextureId(TextureHandle) const override {
			return 4242;
		}
	} device;

	Fixture fixture;
	fixture.resources.SetContext({.device = &device});
	const auto id = editor::CreateSceneResource(fixture.scene, fixture.resources, "ImageTexture");
	auto texture = fixture.resources.Get<ImageTexture>(id);
	texture->Adopt(fixture.resources.Context(), TextureHandle{1, 1}, 64, 64);
	ImGui::CreateContext();
	auto &io = ImGui::GetIO();
	io.IniFilename = nullptr;
	io.DisplaySize = {640, 720};
	io.DeltaTime = 1.f / 60;
	unsigned char *pixels;
	int width, height;
	io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
	io.Fonts->SetTexID(ImTextureID{1});
	editor::ResourceInspector inspector;
	for (bool flip : {false, true}) {
		texture->flipVertically = flip;
		// Auto-sized child panels need a measurement frame before their first draw.
		for (int pass = 0; pass < 2; ++pass) {
			ImGui::NewFrame();
			ImGui::SetNextWindowPos({0, 0});
			ImGui::SetNextWindowSize({240, 650});
			ImGui::Begin("Inspector");
			const float left = ImGui::GetCursorScreenPos().x;
			ImGui::PushID("texture");
			ImGui::GetStateStorage()->SetBool(ImGui::GetID("###Properties"), true);
			ImGui::PopID();
			inspector.Draw("texture", id, "ImageTexture", fixture.scene, fixture.resources, nullptr,
				fixture.documents, true);
			ImGui::End();
			ImGui::Render();
			bool found = false;
			for (const auto *list : ImGui::GetDrawData()->CmdLists) {
				for (const auto &command : list->CmdBuffer) {
					if (command.GetTexID() != 4242) {
						continue;
					}
					found = true;
					const auto index = list->IdxBuffer[command.IdxOffset] + command.VtxOffset;
					const auto &topLeft = list->VtxBuffer[index];
					const auto &bottomRight = list->VtxBuffer[index + 2];
					CHECK(topLeft.pos.x > left);
					CHECK(bottomRight.pos.x < left + 224.f);
					CHECK(topLeft.uv.x == 0);
					CHECK(topLeft.uv.y == 1);
					CHECK(bottomRight.uv.x == 1);
					CHECK(bottomRight.uv.y == 0);
				}
			}
			if (pass == 1) {
				CHECK(found);
			}
		}
	}
	ImGui::DestroyContext();
}

TEST_CASE("Resource inspector: shutdown releases all edit history references before resources") {
	Fixture fixture;
	const auto id = editor::CreateSceneResource(fixture.scene, fixture.resources, "TestMaterial");
	auto material = fixture.resources.Get<TestMaterial>(id);
	for (int edit = 0; edit < 5; ++edit) {
		const auto before = editor::SaveResource(*material);
		material->strength += 1;
		REQUIRE(editor::RecordResourceEdit(fixture.documents, fixture.scene, fixture.resources, id,
			before, editor::SaveResource(*material)));
	}
	CHECK(fixture.resources.OwnerCount(id) == 6); // scene plus five history entries
	SUBCASE("All edits applied") {
	}
	SUBCASE("Some edits undone") {
		REQUIRE(editor::UndoSceneDocument(fixture.documents, fixture.scene));
		REQUIRE(editor::UndoSceneDocument(fixture.documents, fixture.scene));
	}

	Log::Clear();
	// Editor::Shutdown clears document history before Application clears resources.
	fixture.documents.clear();
	CHECK(fixture.resources.OwnerCount(id) == 1);
	CHECK(fixture.resources.Has(id));
	fixture.resources.Clear();
	// Repeated shutdown and subsequent member destruction must be harmless.
	fixture.documents.clear();
	CHECK_FALSE(fixture.resources.Has(id));
	for (const auto &message : Log::GetLines()) {
		CHECK(message.severity != Log::Severity::Error);
	}
}

TEST_CASE("Resource inspector: long string edits preserve content through undo and redo") {
	struct TextAsset : Resource {
		std::string text;

		void Reload(const ResourceContext &) override {
		}
	};

	Fixture fixture;
	fixture.resources.RegisterType<TextAsset>("TextAsset").Field("text", &TextAsset::text);
	const auto id = editor::CreateSceneResource(fixture.scene, fixture.resources, "TextAsset");
	auto resource = fixture.resources.Get<TextAsset>(id);
	std::string original;
	SUBCASE("empty field grows") {
		original = "";
	}
	SUBCASE("existing content exceeds old buffer") {
		original = std::string(4096, 'x') + "TAIL";
	}
	resource->text = original;

	ImGui::CreateContext();

	struct Cleanup {
		~Cleanup() {
			ImGui::DestroyContext();
		}
	} cleanup;

	auto &io = ImGui::GetIO();
	io.IniFilename = nullptr;
	io.DisplaySize = {800, 600};
	io.DeltaTime = 1.f / 60;
	unsigned char *pixels;
	int width, height;
	io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
	editor::ResourceInspector inspector;
	auto frame = [&] {
		ImGui::NewFrame();
		ImGui::SetNextWindowPos({0, 0});
		ImGui::SetNextWindowSize({720, 400});
		ImGui::Begin("String editing");
		ImGui::PushID("text");
		ImGui::GetStateStorage()->SetBool(ImGui::GetID("###Properties"), true);
		ImGui::PopID();
		inspector.Draw("text", id, "TextAsset", fixture.scene, fixture.resources, nullptr,
			fixture.documents, true);
		ImGui::End();
		ImGui::Render();
	};
	frame();
	frame();
	ImGuiWindow *properties = nullptr;
	for (auto *window : ImGui::GetCurrentContext()->Windows) {
		if (std::string_view(window->Name).find("##resource_properties") !=
			std::string_view::npos) {
			properties = window;
		}
	}
	REQUIRE(properties);
	io.AddMousePosEvent(properties->DC.CursorStartPos.x + 20.f,
		properties->DC.CursorStartPos.y + ImGui::GetTextLineHeightWithSpacing() + 5.f);
	io.AddMouseButtonEvent(0, true);
	frame();
	io.AddMouseButtonEvent(0, false);
	frame();
	REQUIRE(ImGui::GetIO().WantTextInput);
	io.AddKeyEvent(ImGuiKey_End, true);
	frame();
	io.AddKeyEvent(ImGuiKey_End, false);
	frame();
	const std::string added = std::string(5000, 'y') + "END";
	io.AddInputCharactersUTF8(added.c_str());
	frame();
	REQUIRE(resource->text == original + added);
	io.AddMousePosEvent(790, 590);
	io.AddMouseButtonEvent(0, true);
	frame();
	io.AddMouseButtonEvent(0, false);
	frame();
	inspector.Commit(fixture.resources, fixture.documents);
	REQUIRE(editor::UndoSceneDocument(fixture.documents, fixture.scene));
	CHECK(resource->text == original);
	REQUIRE(editor::RedoSceneDocument(fixture.documents, fixture.scene));
	CHECK(resource->text == original + added);
}
