#include "gui_server.hxx"
#include "rendering_server.hxx"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "utils/utils.hxx"

#include "file_server.hxx"

GuiServer &GuiServer::get() {
	static GuiServer *server = new GuiServer;
	return *server;
}

static bool begin_footer(const std::string &label);
static void end_footer();

static void set_style(ImGuiStyle &style);

void GuiServer::Initialize() {
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO &io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	ImGui::StyleColorsDark();

	ImGui_ImplGlfw_InitForOpenGL(RenderingServer::get().m_pWindowHandle, true);
	ImGui_ImplOpenGL3_Init("#version 130");

	file_buffer font;
	if (FileServer::get().ReadFileBytes("res://NotoSansKR-Medium.otf", font) == OK) {
		ImVector<ImWchar> ranges;
		ImFontGlyphRangesBuilder builder;
		builder.AddRanges(io.Fonts->GetGlyphRangesDefault());
		//
		builder.BuildRanges(&ranges);

		io.Fonts->AddFontFromMemoryTTF(font.data(), font.size(), 18, nullptr, ranges.Data);

		file_buffer font2;
		if (FileServer::get().ReadFileBytes("res://Font Awesome 6 Free-Regular-400.otf", font2) == OK) {
			static ImWchar ranges[] = {0xf000, 0xf3ff, 0};

			ImFontConfig config;
			config.MergeMode = true;

			io.Fonts->AddFontFromMemoryTTF(font2.data(), font2.size(), 14, &config, ranges);
		}

		io.Fonts->Build();
	}

	ImGuiStyle &style = ImGui::GetStyle();
	set_style(style);
}

void GuiServer::Begin() {
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
}

void GuiServer::Update() {
	static bool show_style = false;
	static bool show_demo = false;

	ImGuiWindowFlags windowFlags = 0;
	windowFlags |= ImGuiWindowFlags_NoCollapse;

	vec2 winSize = RenderingServer::get().GetWindowSize();

	ImGui::SetNextWindowPos(ImVec2(0.f, 0.f), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(winSize.x, winSize.y), ImGuiCond_Always);

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
	ImGui::Begin("Editor", nullptr, windowFlags | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBringToFrontOnFocus);
	ImGui::PopStyleVar();

	ImGui::DockSpace(ImGui::GetID("Editor"), ImVec2(0.f, 0.f), ImGuiDockNodeFlags_PassthruCentralNode);
	if (ImGui::BeginMainMenuBar()) {
		if (ImGui::BeginMenu("  File  ")) {

			ImGui::Checkbox("Show Style", &show_style);
			ImGui::Checkbox("Show Demo", &show_demo);
			ImGui::EndMenu();
		}

		ImGui::EndMainMenuBar();
	}

	if (ImGui::Begin("Viewport", nullptr, windowFlags)) {

		float width = ImGui::GetContentRegionAvail().x;
		float button_sz = 36.f;

		ImGui::SetWindowFontScale(1.25f);
		ImGui::SetCursorPosX(width * 0.5f - button_sz * 0.5f);
		if (App::get().IsRunning()) {
			if (ImGui::Button("\uf28b", ImVec2(button_sz, button_sz))) {
				App::get().Stop();
			}
		} else {
			if (ImGui::Button("\uf144", ImVec2(button_sz, button_sz))) {
				App::get().Start();
			}
		}
		ImGui::SetWindowFontScale(1.f);
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2.f);

		ImVec2 pos;
		ImVec2 size;

		ImVec2 windowSize = ImGui::GetContentRegionAvail();

		float windowRatio = (float)windowSize.x / windowSize.y;
		float videoRatio = (float)1920.f / 1080.f;

		if (windowRatio > videoRatio) {
			float width = windowSize.y * videoRatio;
			float height = windowSize.y;
			float gap = windowSize.x - width;

			float x = gap / 2.f;

			pos.x = x;
			pos.y = 0.f;
			size.x = width;
			size.y = height;

		} else {
			float width = windowSize.x;
			float height = windowSize.x / videoRatio;
			float gap = windowSize.y - height;

			float y = gap / 2.f;

			pos.x = 0.f;
			pos.y = y;
			size.x = width;
			size.y = height;
		}

		// pos.y += ImGui::GetFrameHeight();
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + pos.x);
		// ImGui::SetCursorPosY(ImGui::GetCursorPosY() + pos.y);

		ImVec2 windowPos = ImGui::GetWindowPos();
		ImVec2 cursor = ImGui::GetCursorPos();
		ImGui::Image((ImTextureID)1, size, ImVec2(0.f, 1.f), ImVec2(1.f, 0.f));

		m_viewport_rect = rect(windowPos.x + cursor.x, windowPos.y + cursor.y, size.x, size.y);
		on_viewport = ImGui::IsItemHovered();
	}
	ImGui::End();
	if (ImGui::Begin("Filesystem", nullptr, windowFlags)) {
		std::function<void(const std::vector<FileEntry>)> func;

		func = [&](const std::vector<FileEntry> &entries) {
			for (auto &entry : entries) {

				if (entry.IsDirectory()) {
					bool tree_open = ImGui::TreeNodeEx(("\uf07b   " + entry.Path().filename().string()).c_str(), ImGuiTreeNodeFlags_SpanFullWidth);
					if (ImGui::IsMouseClicked(ImGuiMouseButton_Right) && ImGui::IsItemHovered()) {
						ImGui::OpenPopup("popup_filesystem_rclick");
					}
					if (tree_open) {

						auto childEntries = FileServer::get().ReadDir(entry.Path().string().c_str());
						func(childEntries);

						ImGui::TreePop();
					}
				}
			}

			for (auto &entry : entries) {
				if (entry.IsFile()) {
					if (ImGui::TreeNodeEx(("\uf15b   " + entry.Path().filename().string()).c_str(), ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_Leaf)) {

						ImGui::TreePop();
					}
				}
			}
		};

		auto entries = FileServer::get().ReadDir("res://", false);
		func(entries);

		if (ImGui::BeginPopup("popup_filesystem_rclick")) {
			ImGui::MenuItem("Open Containing Folder");

			ImGui::EndPopup();
		}
	}
	ImGui::End();

	if (ImGui::Begin("Terminal")) {
		static bool to_bottom = true;

		ImVec2 childArea = ImGui::GetContentRegionAvail();
		childArea.y -= ImGui::GetFrameHeight() * 2;

		ImGui::BeginChild("command_text_area", childArea);

		if (to_bottom) {
			to_bottom = false;
			ImGui::SetScrollHereY(1.f);
		}

		ImGui::EndChild();
		static char buf[256] = {0};

		ImGui::SetNextItemWidth(childArea.x);
		if (ImGui::InputText("##command_line", buf, 256, ImGuiInputTextFlags_EnterReturnsTrue)) {
			ImGui::SetKeyboardFocusHere(-1);
			to_bottom = true;

			memset(buf, 0, 256);
		}
	}
	ImGui::End();

	if (ImGui::Begin("Scene")) {
		if (Scene *scene = App::get().GetCurrentScene(); nullptr != scene) {
			std::function<void(Node *)> drawNode;

			drawNode = [&](Node *node) {
				ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
				if (node->get_children().size() == 0) {
					flags |= ImGuiTreeNodeFlags_Leaf;
				}
				if (App::get().GetSelectedNode() == node->id()) {
					flags |= ImGuiTreeNodeFlags_Selected;
				}

				bool open = ImGui::TreeNodeEx(("\uf248   " + node->name()).c_str(), flags);
				if (!ImGui::IsItemToggledOpen() && ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
					App::get().SelectNode(node->id());
				}
				if (open) {

					for (Node *child : node->get_children()) {
						drawNode(child);
					}

					ImGui::TreePop();
				}
			};

			for (Node *node : scene->Nodes()) {
				drawNode(node);
			}
		}
	}
	ImGui::End();

	if (ImGui::Begin("Properties")) {
		if (ImGui::BeginChild("##Properties_", ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y - ImGui::GetFrameHeight()))) {
			if (size_t id = App::get().GetSelectedNode(); id != 0 && nullptr != App::get().GetCurrentScene()) {
				Node *node = App::get().GetCurrentScene()->get_node_by_id(id);
				if (nullptr != node) {
					ImGui::Text("%s : %s", node->name().c_str(), NodeDB::get().get_node_typename(node->type_hash()).c_str());

					std::vector<std::string> propNames;
					NodeType nodeType = node->type_hash();

					NodeDB::get().get_property_names(nodeType, propNames);

					for (const auto &propName : propNames) {
						ImGui::Text("%s", propName.c_str());

						auto prop = NodeDB::get().get_property(nodeType, propName);
						if (prop.typeName == "vec2") {
							Property p = prop.get_ref(node);
							if (vec2 **val = std::any_cast<vec2 *>(&p); nullptr != val) {
								ImGui::DragFloat2(("##" + propName).c_str(), &((*val)->x));
							}
						}
						if (prop.typeName == "bool") {
							Property p = prop.get_ref(node);
							if (bool **val = std::any_cast<bool *>(&p); nullptr != val) {
								ImGui::Checkbox(("##" + propName).c_str(), *val);
							}
						} else if (prop.typeName == "float") {
							Property p = prop.get_ref(node);
							if (float **val = std::any_cast<float *>(&p); nullptr != val) {
								if (propName != "rotation") {
									ImGui::DragFloat(("##" + propName).c_str(), *val);
								} else {
									float slider = **val;
									slider = glm::radians(slider);

									ImGui::SliderAngle(("##" + propName).c_str(), &slider);
									**val = glm::degrees(slider);
								}
							}
						} else if (prop.typeName == "std::string") {
							Property p = prop.get_ref(node);
							if (std::string **val = std::any_cast<std::string *>(&p); nullptr != val) {
								static char buf[512];
								memset(buf, 0, 512);
								memcpy(buf, (*val)->data(), std::min((*val)->size(), static_cast<size_t>(512)));

								if (ImGui::InputText(("##" + propName).c_str(), buf, 512, ImGuiInputTextFlags_EnterReturnsTrue)) {
								}
								(**val) = buf;
							}
						} else if (prop.typeName == "RID") {
							Property p = prop.get_ref(node);
							if (RID **val = std::any_cast<RID *>(&p); nullptr != val) {
								ImGui::InputInt(("##" + propName).c_str(), *val, 1, 100);
							}
						} else {
							ImGui::NewLine();
						}
					}
				}
			}
		}
		ImGui::EndChild();
	}
	ImGui::End();

	if (show_style) {
		if (ImGui::Begin("Style", nullptr, windowFlags)) {
			ImGui::ShowStyleEditor();
		}
		ImGui::End();
	}

	if (show_demo) {
		ImGui::ShowDemoWindow(nullptr);
		ImGui::End();
	}

	ImGui::End();

	begin_footer("Footer");
	ImGui::Text("Sowa Engine | Lexographics");
	end_footer();
}

void GuiServer::End() {
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

bool begin_footer(const std::string &label) {
	const ImGuiViewport *viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(
		ImVec2(viewport->Pos.x,
			   viewport->Pos.y + viewport->Size.y - ImGui::GetFrameHeight()));
	ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, ImGui::GetFrameHeight()));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.f);

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

void end_footer() {
	ImGui::EndMenuBar();
	ImGui::End();
	ImGui::PopStyleColor(2);
}

void set_style(ImGuiStyle &style) {
	style.WindowMenuButtonPosition = ImGuiDir_None;
	style.TabRounding = 0.f;
	style.GrabRounding = 2.3f;
	style.FrameRounding = 2.3f;
}