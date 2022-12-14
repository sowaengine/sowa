#include "Servers/ScriptServer/LuaScriptServer.hpp"

#include "Resource/Resource.hpp"
#include "Resource/Texture/Texture.hpp"
#include "Servers/GuiServer/GuiServer.hpp"

#include "Resource/ResourceLoader.hpp"

#include "Utils/Dialog.hpp"
#include "Utils/String.hpp"

namespace Sowa {

void LuaScriptServer::RegisterGuiServer() {
	_pState->new_enum("gui_window_flags",
					  "none", WindowFlags_None,
					  "no_resize", WindowFlags_NoResize,
					  "no_move", WindowFlags_NoMove,
					  "no_bring_to_front_on_focus", WindowFlags_NoBringToFrontOnFocus,
					  "no_nav_focus", WindowFlags_NoNavFocus,
					  "no_docking", WindowFlags_NoDocking,
					  "no_title_bar", WindowFlags_NoTitleBar,
					  "no_collapse", WindowFlags_NoCollapse,
					  "menu_bar", WindowFlags_MenuBar,
					  "no_background", WindowFlags_NoBackground);

	_pState->new_enum("gui_style_var",
					  "none", StyleVar::None,
					  "window_padding", StyleVar::WindowPadding,
					  "window_rounding", StyleVar::WindowRounding);

	_pState->new_enum("gui_mouse_button",
					  "none", GuiMouseButton::None,
					  "left", GuiMouseButton::Left,
					  "right", GuiMouseButton::Right,
					  "middle", GuiMouseButton::Middle);

	_pState->new_usertype<GuiServer>(
		"GuiServer",

		"new", sol::no_constructor,
		"get", [this]() -> Sowa::GuiServer & { return *this->_Context.GetSingleton<Sowa::GuiServer>(Sowa::Server::GUISERVER); },

		"begin_window", &GuiServer::BeginWindow,
		"end_window", &GuiServer::EndWindow,

		"begin_child",
		sol::overload(
			&GuiServer::BeginChild,
			[](GuiServer &self, const std::string &id) { return self.BeginChild(id, 0, 0); },
			[](GuiServer &self, const std::string &id, double w, double h) { return self.BeginChild(id, static_cast<int>(w), static_cast<int>(h)); }),

		"end_child", &GuiServer::EndChild,

		"set_next_window_size", [](GuiServer &self, double w, double h) -> void { self.SetNextWindowSize(static_cast<int>(w), static_cast<int>(h)); },
		"set_next_window_pos", [](GuiServer &self, double x, double y) -> void { self.SetNextWindowPos(static_cast<int>(x), static_cast<int>(y)); },

		"push_style_var",
		sol::overload(
			[](GuiServer &self, StyleVar var, double value) -> void { self.PushStyleVar(var, static_cast<float>(value)); },
			[](GuiServer &self, StyleVar var, Vec2 value) -> void { self.PushStyleVar(var, value); }),

		"pop_style_var", &GuiServer::PopStyleVar,

		"begin_main_menu_bar", &GuiServer::BeginMainMenuBar,
		"end_main_menu_bar", &GuiServer::EndMainMenuBar,
		"begin_menu", &GuiServer::BeginMenu,
		"end_menu", &GuiServer::EndMenu,
		"menu_item",
		sol::overload(
			&GuiServer::MenuItem,
			[](GuiServer &self, const std::string &label) -> bool { return self.MenuItem(label, ""); }),

		"register_panel", &GuiServer::RegisterPanel,
		"draw_view_bar", &GuiServer::DrawViewbar,

		"begin_table", &GuiServer::BeginTable,
		"table_next_row", &GuiServer::TableNextRow,
		"table_next_column", &GuiServer::TableNextColumn,
		"end_table", &GuiServer::EndTable,

		"check_box", [](GuiServer &self, const std::string &label, bool value, sol::this_state L) -> sol::variadic_results {
				bool val = value;
				bool ret = self.Checkbox(label, val);
				
				sol::variadic_results values;
				values.push_back( {L, sol::in_place_type<bool>, val} );
				values.push_back( {L, sol::in_place_type<bool>, ret} );

				return values; },

		"input_text", [](GuiServer &self, const std::string &label, const std::string &value, sol::this_state L) -> sol::variadic_results {
			std::string val = value;
			bool ret = self.InputText(label, val);

			sol::variadic_results values;
			values.push_back( {L, sol::in_place_type<std::string>, val} );
			values.push_back( {L, sol::in_place_type<bool>, ret} );

			return values; },

		"drag_float",
		sol::overload(
			[](GuiServer &self, const std::string &label, float value, sol::this_state L) -> sol::variadic_results {
			float val = value;
			bool ret = self.DragFloat(label, val);

			sol::variadic_results values;
			values.push_back( {L, sol::in_place_type<float>, val} );
			values.push_back( {L, sol::in_place_type<bool>, ret} );

			return values; },
			[](GuiServer &self, const std::string &label, float value, float speed, sol::this_state L) -> sol::variadic_results {
			float val = value;
			bool ret = self.DragFloat(label, val, speed);

			sol::variadic_results values;
			values.push_back( {L, sol::in_place_type<float>, val} );
			values.push_back( {L, sol::in_place_type<bool>, ret} );

			return values; }),

		"drag_float2",
		sol::overload(
			[](GuiServer &self, const std::string &label, Vec2 value, sol::this_state L) -> sol::variadic_results {
			Vec2 val = value;
			bool ret = self.DragFloat2(label, val.x, val.y);

			sol::variadic_results values;
			values.push_back( {L, sol::in_place_type<Vec2>, val} );
			values.push_back( {L, sol::in_place_type<bool>, ret} );

			return values; },
			[](GuiServer &self, const std::string &label, Vec2 value, float speed, sol::this_state L) -> sol::variadic_results {
			Vec2 val = value;
			bool ret = self.DragFloat2(label, val.x, val.y, speed);

			sol::variadic_results values;
			values.push_back( {L, sol::in_place_type<Vec2>, val} );
			values.push_back( {L, sol::in_place_type<bool>, ret} );

			return values; }),

		"setup_dockspace", &GuiServer::SetupDockspace,

		"begin_footer", &GuiServer::BeginFooter,
		"end_footer", &GuiServer::EndFooter,

		"text", &GuiServer::Text,
		"text_unformatted", &GuiServer::TextUnformatted,

		"show_demo_window", &GuiServer::ShowDemoWindow,

		"draw_filesystem", &GuiServer::DrawFilesystem,
		"draw_play_button", &GuiServer::DrawPlayButton,
		"separator", &GuiServer::Separator,
		"draw_frame", &GuiServer::DrawFrame,

		"draw_scene", &GuiServer::DrawScene,

		"is_window_hovered", &GuiServer::IsWindowHovered,
		"is_mouse_clicked", &GuiServer::IsMouseClicked,
		"is_mouse_pressed", &GuiServer::IsMousePressed,

		"open_context_menu", &GuiServer::OpenContextMenu,
		"begin_context_menu", &GuiServer::BeginContextMenu,
		"end_context_menu", &GuiServer::EndContextMenu,

		"get_available_width", &GuiServer::GetAvailableWidth,
		"get_available_height", &GuiServer::GetAvailableHeight,
		"get_title_height", &GuiServer::GetTitleHeight,

		"set_scroll_ratio_y", &GuiServer::SetScrollRatioY,

		"same_line", &GuiServer::SameLine,

		"header", &GuiServer::Header,
		"indent",
		sol::overload(&GuiServer::Indent,
					  [](GuiServer &self) { self.Indent(0.f); }),

		"unindent",
		sol::overload(&GuiServer::Unindent,
					  [](GuiServer &self) { self.Unindent(0.f); }),

		"get_mouse_position", &GuiServer::GetMousePosition,

		"resource_picker", [this](GuiServer &self, const std::string &label, uint32_t resourceType, sol::variadic_args resource, sol::this_state L) -> sol::variadic_results {
			sol::variadic_results res;


			if(resourceType == Resource_ImageTexture) {
				Reference<ImageTexture> tex = nullptr;
				
				auto tex_res = resource[0];
				if(tex_res.is<Reference<ImageTexture>>())
					tex = tex_res.as<Reference<ImageTexture>>();


				self.PushID(label);

				self.BeginTable("resource_picker", 2);
				self.TableNextRow();
				self.TableNextColumn();
				self.Text(label);
				if (self.Button("Remove")) {
					res.clear(); // remove if there any Resource pushed
					res.push_back({L, sol::in_place_type<Reference<ImageTexture>>, nullptr});
				}
				self.TableNextColumn();
				if(self.ImageButton(tex, 64, 64)) {
					bool isRelative = false;
					std::string path = Dialog::OpenFileDialog("Pick new texture", File::Path("res://").string(), 0, {}, false, true, &isRelative);

					if(!isRelative) {
						Debug::Error("Loaded resource must be in project directory");
					} else {
						Reference<Scene> scene = this->_Context.GetSingleton<Sowa::Application>(Sowa::Server::APPLICATION)->GetCurrentScene();
						Reference<ImageTexture> loadedTex = scene->GetResourceManager<ImageTexture>().LoadResource(Format("res://{}", path).c_str());
						res.push_back( { L, sol::in_place_type<Reference<ImageTexture>>, loadedTex } );
					}
				}

				self.EndTable();
				self.PopID();
			}

			
			if(res.size() == 0) {
				res.push_back( sol::nil );
			}

			return res; },

		"set_cursor_pos_x", &GuiServer::SetCursorPosX,
		"set_cursor_pos_y", &GuiServer::SetCursorPosY,
		"get_cursor_pos_x", &GuiServer::GetCursorPosX,
		"get_cursor_pos_y", &GuiServer::GetCursorPosY,

		"push_id", &GuiServer::PushID,
		"pop_id", &GuiServer::PopID,

		"button",
		sol::overload([](GuiServer &self, const std::string &label) -> bool { return self.Button(label); },
					  [](GuiServer &self, const std::string &label, double width, double height) -> bool { return self.Button(label, static_cast<int>(width), static_cast<int>(height)); }));
}
} // namespace Sowa
