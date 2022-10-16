import void Scene() from "Ease.Editor.Panels";
import void Console() from "Ease.Editor.Panels";
import void Properties() from "Ease.Editor.Panels";
import void Filesystem() from "Ease.Editor.Panels";
import void Game() from "Ease.Editor.Panels";


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
    panels.insertLast(Panel("Scene", @Scene));
    panels.insertLast(Panel("Properties", @Properties));
    panels.insertLast(Panel("Console", @Console));
    panels.insertLast(Panel("Filesystem", @Filesystem));
    panels.insertLast(Panel("Game", @Game));

    for(uint i=0; i<panels.length(); i++) {
        Gui::RegisterPanel(panels[i].panelName, panels[i].panelName);
    }

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
    Gui::SetNextWindowSize(int(windowSize.x), int(windowSize.y));
    Gui::SetNextWindowPos(0, 0);
    
    Gui::PushStyleVar(Gui::StyleVar_WindowPadding, Vector2(0.0, 0.0));
    Gui::PushStyleVar(Gui::StyleVar_WindowRounding, 0.0);
    bool window_editor = Gui::BeginWindow("Editor",
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

    if(window_editor) {
        if(Gui::BeginMainMenuBar()) {
            if(Gui::BeginMenu("File")) {
                if(Gui::BeginMenu("New")) {
                    Gui::MenuItem("Scene");

                    Gui::EndMenu();
                }
                Gui::MenuItem("Import/Open File", "CTRL+O");
                Gui::MenuItem("Quit to Project Manager", "");

                Gui::EndMenu();
            }

            if(Gui::BeginMenu("View")) {
                Gui::DrawViewbar();

                Gui::EndMenu();
            }

            if(Gui::BeginMenu("Project")) {
                Gui::MenuItem("Project Settings");
                Gui::MenuItem("Export");

                Gui::EndMenu();
            }

            if(Gui::BeginMenu("Debug")) {
                bool a;
                Gui::Checkbox("Visible Collision Shapes", a);

                Gui::EndMenu();
            }

            Gui::EndMainMenuBar();
        }
        Gui::SetupDockspace();


        if(Gui::BeginFooter("##statusbar")) {
            Gui::Text("Sowa Engine | Lexographics");
        }
        Gui::EndFooter();

        Gui::ShowDemoWindow();
        for(uint i=0; i<panels.length(); i++) {
            if(Gui::BeginWindow(panels[i].panelName)) {
                panels[i].func();
            }
            Gui::EndWindow();
            
        }

    }
    Gui::EndWindow();
}