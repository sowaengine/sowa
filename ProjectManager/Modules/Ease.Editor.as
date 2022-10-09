

void init()
{
    Debug::Log("Init");

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
    
    Gui::PushStyleVar(Gui::StyleVar::WindowPadding, Vector2(0.0, 0.0));
    Gui::PushStyleVar(Gui::StyleVar::WindowRounding, 0.0);
    Gui::BeginWindow("Editor", Gui::WindowFlags::NoResize | Gui::WindowFlags::NoMove | Gui::WindowFlags::NoBringToFrontOnFocus);
    Gui::PopStyleVar(2);

    Gui::Text("Window1 text");
    Gui::EndWindow();

    Gui::BeginWindow("Window2");
    Gui::Text("Window2 text");
    if(Gui::Button("Press")) {
        Debug::Log("Pressed");
    }
    
    if(Gui::DragFloat("Value", x)) {
        Debug::Log("Dragging");
    }
    Gui::EndWindow();
}