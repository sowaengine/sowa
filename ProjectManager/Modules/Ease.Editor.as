

funcdef void DrawPanelFunc();
class Panel {
    string panelName;
    DrawPanelFunc@ func;

    Panel(string _panelName, DrawPanelFunc@ _func) {
        panelName = _panelName;
        @func = _func;
    }
}

array<Panel@> panels;

void PanelFunc() {

}


void init()
{
    Debug::Log("Init");

    panels.insertLast(Panel("Scene", @PanelFunc));
    panels.insertLast(Panel("Inspector", @PanelFunc));
    panels.insertLast(Panel("Console", @PanelFunc));
    panels.insertLast(Panel("Filesystem", @PanelFunc));
    panels.insertLast(Panel("Game", @PanelFunc));

    // Debug
    if(false)
    {
        Debug::Log("Formatted Text {}", 11);
        Debug::Info("Formatted Text {}", 22);
        Debug::Warn("Formatted Text {}", 33);
        Debug::Error("Formatted Text {}", 44);
    }

    // Vector2
    if(false)
    {
        Vector2 vec1;
        Vector2 vec2(10.f);
        Vector2 vec3(20.f, 30.f);
        vec1.x = 20;
        vec2.y = vec3.x;

        // Debug::Log("vec1: {}, vec2: {}, vec3: {}", vec1, vec2, vec3);
    }

    if(false)
    {
        Vector2 clamped = Vector2(2, 2).Clamp(200.0f);
        Debug::Log("Clamped: {}, Length: {}, Angle: {}", clamped, clamped.Length(), clamped.Angle());
    }
}

void update()
{
    
}

float x = 0.0;
void gui_update()
{
    Vector2 windowSize = Window::GetWindowSize();
    Gui::SetNextWindowSize(windowSize.x, windowSize.y);
    Gui::SetNextWindowPos(0, 0);
    
    Gui::PushStyleVar(Gui::StyleVar_WindowPadding, Vector2(0.0, 0.0));
    Gui::PushStyleVar(Gui::StyleVar_WindowRounding, 0.0);
    Gui::BeginWindow("Editor",
        Gui::WindowFlags_NoResize              |
        Gui::WindowFlags_NoMove                |
        Gui::WindowFlags_NoBringToFrontOnFocus |
        Gui::WindowFlags_NoNavFocus            |
        Gui::WindowFlags_NoDocking             |
        Gui::WindowFlags_NoTitleBar            |
        Gui::WindowFlags_NoCollapse            |
        Gui::WindowFlags_MenuBar               |
        Gui::WindowFlags_NoBackground);
    Gui::PopStyleVar(2);

    if(Gui::BeginMainMenuBar()) {
        if(Gui::BeginMenu("View")) {
            for(uint i=0; i<4; i++) {
                if(Gui::MenuItem("MenuItem " + i))
                    Debug::Log("Pressed MenuItem " + i);
            }

            Gui::EndMenu();
        }

        Gui::EndMainMenuBar();
    }


    Gui::SetupDockspace();

    if(Gui::BeginFooter("##statusbar")) {
    }
    Gui::EndFooter();

    for(uint i=0; i<panels.length(); i++)
    {
        Gui::BeginWindow(panels[i].panelName);
        panels[i].func();
        Gui::EndWindow();
    }

    Gui::EndWindow();
}