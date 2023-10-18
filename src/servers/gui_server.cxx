#include "gui_server.hxx"
#include "rendering_server.hxx"

#include "scene/nodes/2d/physics/physics_body_2d.hxx"

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

	bool scene_rclick_open_popup = false;

	if (ImGui::Begin("Scene")) {
		if (Scene *scene = App::get().GetCurrentScene(); nullptr != scene) {
			std::function<void(Node *)> drawNode;

			drawNode = [&](Node *node) {
				if (node == nullptr)
					return;

				ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
				if (node->get_children().size() == 0) {
					flags |= ImGuiTreeNodeFlags_Leaf;
				}
				if (App::get().GetSelectedNode() == node->id()) {
					flags |= ImGuiTreeNodeFlags_Selected;
				}

				bool open = ImGui::TreeNodeEx(("\uf248   " + node->name()).c_str(), flags);
				if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
					scene_rclick_selected_node = node->id();
					scene_rclick_open_popup = true;
				}

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

			drawNode(scene->Root());

			if (!ImGui::IsAnyItemHovered() && ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
				scene_rclick_selected_node = 0;
				scene_rclick_open_popup = true;
			}

			if (scene_rclick_open_popup) {
				ImGui::OpenPopup("##scene_rclick");
			}

			if (ImGui::BeginPopup("##scene_rclick")) {
				if (ImGui::BeginMenu("Add Node")) {
					std::function<void(const NodeTypeName &)> drawNodeTypeName;

					auto addNodeByTypeName = [this](const std::string &typeName) {
						Node *node = App::get().GetCurrentScene()->create(NodeDB::get().get_node_type(typeName), "New " + typeName);

						if (scene_rclick_selected_node == 0) {
							if (nullptr == App::get().GetCurrentScene()->Root()) {
								App::get().GetCurrentScene()->Root() = (node);
							} else {
								App::get().GetCurrentScene()->Root()->add_child(node);
							}
						} else {
							App::get().GetCurrentScene()->get_node_by_id(scene_rclick_selected_node)->add_child(node);
						}

						App::get().SelectNode(node->id());
					};

					drawNodeTypeName = [&](const NodeTypeName &nodeTypeName) {
						if (nodeTypeName.children.size() == 0) {
							if (ImGui::Selectable(nodeTypeName.name.c_str())) {
								addNodeByTypeName(nodeTypeName.name);
							}

						} else {
							if (ImGui::BeginMenu(nodeTypeName.name.c_str())) {
								if (nodeTypeName.addable_to_scene && ImGui::IsItemClicked()) {
									addNodeByTypeName(nodeTypeName.name);
									ImGui::CloseCurrentPopup();
								}

								for (const NodeTypeName &nt : nodeTypeName.children) {
									drawNodeTypeName(nt);
								}

								ImGui::EndMenu();
							}
						}
					};

					for (const NodeTypeName &nt : App::get().NodeTypeNames()) {
						drawNodeTypeName(nt);
					}

					ImGui::EndMenu();
				}

				if (scene_rclick_selected_node != 0) {
					if (ImGui::Selectable("Delete Node")) {
						App::get().GetCurrentScene()->queue_free(scene_rclick_selected_node);
					}
				}

				ImGui::EndPopup();
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

					for (int i = static_cast<int>(propNames.size()) - 1; i >= 0; i--) {
						const auto &propName = propNames[i];

						ImGui::Text("%s", propName.c_str());

						auto prop = NodeDB::get().get_property(nodeType, propName);
						if (prop.typeName == "vec2") {
							Property p = prop.get_ref(node);
							if (vec2 **val = std::any_cast<vec2 *>(&p); nullptr != val) {
								ImGui::DragFloat2(("##" + propName).c_str(), &((*val)->x));
							}
						} else if (prop.typeName == "bool") {
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

						} else if (prop.typeName == "PhysicsBodyType") {
							Property p = prop.get_ref(node);
							if (PhysicsBodyType **val = std::any_cast<PhysicsBodyType *>(&p); nullptr != val) {
								if (ImGui::BeginCombo(("##" + propName).c_str(), PhysicsBodyTypeToString(**val).c_str())) {
									bool t_static = false;
									bool t_kinematic = false;
									bool t_dynamic = false;

									ImGui::Selectable("Static", &t_static);
									ImGui::Selectable("Kinematic", &t_kinematic);
									ImGui::Selectable("Dynamic", &t_dynamic);

									if (t_static) {
										**val = PhysicsBodyType::Static;
									} else if (t_kinematic) {
										**val = PhysicsBodyType::Kinematic;
									} else if (t_dynamic) {
										**val = PhysicsBodyType::Dynamic;
									}

									ImGui::EndCombo();
								}
							}

						} else {
							ImGui::Text("Unknown");
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

	style.Colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
	style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
	style.Colors[ImGuiCol_WindowBg] = ImVec4(0.13f, 0.14f, 0.15f, 1.00f);
	style.Colors[ImGuiCol_ChildBg] = ImVec4(0.13f, 0.14f, 0.15f, 1.00f);
	style.Colors[ImGuiCol_PopupBg] = ImVec4(0.13f, 0.14f, 0.15f, 1.00f);
	style.Colors[ImGuiCol_Border] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
	style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	style.Colors[ImGuiCol_FrameBg] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
	style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.38f, 0.38f, 0.38f, 1.00f);
	style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.67f, 0.67f, 0.67f, 0.39f);
	style.Colors[ImGuiCol_TitleBg] = ImVec4(0.08f, 0.08f, 0.09f, 1.00f);
	style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.08f, 0.08f, 0.09f, 1.00f);
	style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
	style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
	style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
	style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
	style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
	style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
	style.Colors[ImGuiCol_CheckMark] = ImVec4(0.11f, 0.64f, 0.92f, 1.00f);
	style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.11f, 0.64f, 0.92f, 1.00f);
	style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.08f, 0.50f, 0.72f, 1.00f);
	style.Colors[ImGuiCol_Button] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
	style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.38f, 0.38f, 0.38f, 1.00f);
	style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.67f, 0.67f, 0.67f, 0.39f);
	style.Colors[ImGuiCol_Header] = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
	style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
	style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.67f, 0.67f, 0.67f, 0.39f);
	style.Colors[ImGuiCol_Separator] = style.Colors[ImGuiCol_Border];
	style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.41f, 0.42f, 0.44f, 1.00f);
	style.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
	style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.29f, 0.30f, 0.31f, 0.67f);
	style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
	style.Colors[ImGuiCol_Tab] = ImVec4(0.08f, 0.08f, 0.09f, 0.83f);
	style.Colors[ImGuiCol_TabHovered] = ImVec4(0.33f, 0.34f, 0.36f, 0.83f);
	style.Colors[ImGuiCol_TabActive] = ImVec4(0.23f, 0.23f, 0.24f, 1.00f);
	style.Colors[ImGuiCol_TabUnfocused] = ImVec4(0.08f, 0.08f, 0.09f, 1.00f);
	style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.13f, 0.14f, 0.15f, 1.00f);
	style.Colors[ImGuiCol_DockingPreview] = ImVec4(0.26f, 0.59f, 0.98f, 0.70f);
	style.Colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
	style.Colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
	style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
	style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
	style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
	style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
	style.Colors[ImGuiCol_DragDropTarget] = ImVec4(0.11f, 0.64f, 0.92f, 1.00f);
	style.Colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
	style.Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
	style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
}