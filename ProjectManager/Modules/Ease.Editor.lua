local editor = {}
editor.author = "Sowa"
editor.name = "Editor"
editor.version = 1

editor.console_text = ""
editor.panels = {}
editor.game_rclick_start_pos = nil


editor.start = function()
    local gui = GuiServer.get()

    editor.panels.scene = {
        name = "Scene",
        custom_padding = Vector2.new(0, 0),
        func = function()
            gui:draw_scene()

            if gui:is_window_hovered() then
                if gui:is_mouse_clicked(gui_mouse_button.right) then
                    gui:open_context_menu("__CTXMENU_scene_rclick")
                end
            end

            if gui:begin_context_menu("__CTXMENU_scene_rclick") then
                if gui:begin_menu("New") then
                    if gui:menu_item("Sprite2D") then
                        local entity = Application.get():get_current_scene():create("Sprite2D")
                        entity:add_component(Component.Transform2D)
                        entity:add_component(Component.Sprite2D)

                        Application.get().selected_entity = entity
                    end
                    if gui:menu_item("Empty Entity") then
                        local entity = Application.get():get_current_scene():create("New Entity")

                        Application.get().selected_entity = entity
                    end
                    gui:end_menu()
                end
                gui:end_context_menu()
            end
        end
    }
    editor.panels.properties = {
        name = "Properties",
        func = function()
            -- component: Component.Transform2D ...
            -- name: "Transform2D"
            local begin_component = function(entity, component, name)
                local comp = entity:get_component(component)
                if comp ~= nil then
                    local header_value = gui:header(name)

                    if header_value then
                        gui:indent()
                        return comp
                    end
                end

                return nil
            end
            local end_component = function(entity, component, name)
                if gui:button("Remove ") then
                    gui:separator()
                    entity:remove_component(component)
                end
                gui:unindent()
            end
            local app = Application.get()
            if app.selected_entity:valid() then
                local selected = app.selected_entity
                local changed

                selected.name, changed = gui:input_text("Name", selected.name)

                local tc = begin_component(selected, Component.Transform2D, "Transform2D")
                if (tc ~= nil) then
                    tc.position, changed = gui:drag_float2("Position", tc.position)
                    tc.scale, changed = gui:drag_float2("Scale", tc.scale)
                    tc.rotation, changed = gui:drag_float("Rotation", tc.rotation)

                    end_component(selected, Component.Transform2D, "Transform2D")
                end

                local spr = begin_component(selected, Component.Sprite2D, "Sprite2D")
                if (spr ~= nil) then

                    local tex = gui:resource_picker("Texture", Resource.ImageTexture, spr.texture)
                    if tex ~= nil then
                        spr.texture = tex
                    end

                    end_component(selected, Component.Sprite2D, "Sprite2D")
                end
            end


            -- add component menu
            local selected = app.selected_entity
            if selected:valid() and gui:is_window_hovered() then
                if gui:is_mouse_clicked(gui_mouse_button.right) then
                    gui:open_context_menu("__CTXMENU_props_rclick")
                end
            end

            if selected:valid() and gui:begin_context_menu("__CTXMENU_props_rclick") then
                if gui:begin_menu("Add Component") then
                    if not selected:has_component(Component.Transform2D) then
                        if gui:menu_item("Transform2D", "") then
                            selected:add_component(Component.Transform2D)
                        end
                    end

                    if not selected:has_component(Component.Sprite2D) then
                        if gui:menu_item("Sprite2D", "") then
                            selected:add_component(Component.Sprite2D)
                        end
                    end
                    gui:end_menu()
                end
                gui:end_context_menu()
            end
        end
    }
    editor.panels.console = {
        name = "Console",
        func = function()
            if gui:button("Clear") then
                editor.console_text = ""
            end
            gui:same_line()

            if gui:begin_child("consoletext", 0, gui:get_available_height() - gui:get_title_height()) then
                gui:text_unformatted(editor.console_text)

                gui:set_scroll_ratio_y(1.0)
                gui:end_child()
            end

        end
    }
    editor.panels.filesystem = {
        name = "Filesystem",
        func = function()
            gui:draw_filesystem()
        end
    }
    editor.panels.game = {
        name = "Game",
        func = function()
            if gui:is_window_hovered() then
                if gui:is_mouse_pressed(gui_mouse_button.right) then
                    if editor.game_rclick_start_pos == nil then
                        editor.game_rclick_start_pos = gui:get_mouse_position()
                    end
                else
                    editor.game_rclick_start_pos = nil
                end


                if gui:is_mouse_pressed(gui_mouse_button.right) then

                    local dt = Vector2.new(gui:get_mouse_position().x - editor.game_rclick_start_pos.x,
                        gui:get_mouse_position().y - editor.game_rclick_start_pos.y)

                    -- editor camera position should be module.game_rclick_start_pos + dt
                end
            end

            gui:draw_play_button()
            gui:separator()
            gui:draw_frame()

        end
    }
    for key, value in pairs(editor.panels) do
        gui:register_panel(value.name, value.name, true)
    end

    -- Vector2
    if false then
        local vec2 = Vector2.new(3, 4)
        -- vec2.x = 3
        -- vec2.y = 4

        local len = vec2:length()
        local new_vec = vec2:clamp(3.0)
        local angle = vec2:angle()
    end

    -- Scene
    if true then
        local app = Application.get()

        local scene = app:get_current_scene()
        local selected = app.selected_entity
    end

    -- Entity
    if false then
        local entity = App.get_selected_entity()
    end

    -- Component
    if false then
        local entity = App.get_selected_entity()
        if entity.has_component(Component.Transform2D) then
            local tc = entity.get_component(Component.Transform2D)
            tc.position = Vector2.new(220, 270)
        end
    end
end

editor.update = function()

end

editor.gui_update = function()
    local window = Window.get()
    local window_size = window:get_window_size()

    local gui = GuiServer.get()
    gui:set_next_window_size(window_size.x, window_size.y)
    gui:set_next_window_pos(0, 0)

    gui:push_style_var(gui_style_var.window_padding, Vector2.new(0, 0))
    gui:push_style_var(gui_style_var.window_rounding, 0)
    local window_editor = gui:begin_window("Editor",
        gui_window_flags.no_resize |
        gui_window_flags.no_move |
        gui_window_flags.no_bring_to_front_on_focus |
        gui_window_flags.no_nav_focus |
        gui_window_flags.no_docking |
        gui_window_flags.no_title_bar |
        gui_window_flags.no_collapse |
        gui_window_flags.menu_bar)
    -- gui_window_flags.no_background
    gui:pop_style_var(2)


    if window_editor then
        if gui:begin_main_menu_bar() then
            if gui:begin_menu("File") then
                if gui:menu_item("Save Scene") then
                    Application.get():get_current_scene():save()
                end
                if gui:menu_item("Save Scene As") then
                    -- local path = dialog:open_file_dialog()
                    -- if path ~= nil then
                    --      Application:get():get_current_scene():save_to_file(path)
                    -- end
                end
                if gui:begin_menu("New") then
                    gui:menu_item("Scene")

                    gui:end_menu()
                end
                gui:menu_item("Import/Open File", "CTRL-O")
                gui:menu_item("Quit To Project Manager")

                gui:end_menu()
            end

            if gui:begin_menu("View") then
                gui:draw_view_bar()
                gui:end_menu()
            end

            if gui:begin_menu("Project") then
                gui:menu_item("Project Settings")
                gui:menu_item("Export")

                gui:end_menu()
            end

            if gui:begin_menu("Debug") then
                if Debug_draw == nil then
                    Debug_draw = false
                end

                local changed
                Debug_draw, changed = gui:check_box("Visible collision shapes", Debug_draw)
                if changed then
                    print("Hey")
                end

                gui:end_menu()
            end

            gui:end_main_menu_bar()
        end

        gui:setup_dockspace()

        -- panels
        gui:show_demo_window()

        for key, value in pairs(editor.panels) do
            local stylevar_count = 0

            -- custom_padding
            if value.custom_padding ~= nil then
                gui:push_style_var(gui_style_var.window_padding, value.custom_padding)
                stylevar_count = stylevar_count + 1
            end

            local begin_window = gui:begin_window(value.name, 0)

            if stylevar_count > 0 then
                gui:pop_style_var(stylevar_count)
            end

            if begin_window then
                value.func()
            end

            gui:end_window()
        end

        if gui:begin_footer("##statusbar") then
            gui:text("Sowa Engine | Lexographics")
        end
        gui:end_footer()
    end
    gui:end_window()
end

function editor.on_print(message)
    editor.console_text = editor.console_text .. message
end

return editor
