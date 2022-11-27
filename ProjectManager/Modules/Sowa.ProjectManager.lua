local projectmanager = {}
projectmanager.author = "Sowa"
projectmanager.name = "Project Manager"
projectmanager.version = 1


projectmanager.start = function()
    reload_projects()
end

projectmanager.update = function()

end

projectmanager.gui_update = function()
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
        gui_window_flags.no_collapse)
    gui:pop_style_var(2)

    if window_editor then
        gui:set_cursor_pos_x(64)
        gui:set_cursor_pos_y(16)
        if gui:button("    Import Project    ") then
            import_project()
            reload_projects()
        end
        gui:same_line()
        if gui:button("    Reload Projects    ") then
            reload_projects()
        end
        gui:separator()

        draw_projects(gui, window_size.x, window_size.y / 8)

        if gui:begin_footer("##statusbar") then
            gui:text("Sowa Engine | Lexographics")
        end
        gui:end_footer()
    end
    gui:end_window()
end

return projectmanager
