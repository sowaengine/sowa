

void Scene() {
    /*Gui::BeginCheckerList("##sceneview");

    Gui::CheckerListNextItem();
    if(Gui::BeginTree("Entity 1"))
    {
        Gui::CheckerListNextItem();
        if(Gui::BeginTree("Entity 2"))
        {
            Gui::CheckerListNextItem();
            if(Gui::BeginTree("Entity 3", Gui::TreeFlags_Leaf))
            {
                Gui::EndTree();
            }
            Gui::EndTree();
        }
        Gui::EndTree();
    }

    Gui::EndCheckerList();*/
    Gui::DrawScene();


    if(Gui::IsWindowHovered()) {
        if(Gui::IsMouseClicked(Gui::GuiMouseButton_Right)) {
            Gui::OpenContextMenu("__CTXMENU__scene_rclick");
        }
    }
    if(Gui::BeginContextMenu("__CTXMENU__scene_rclick")) {
        if(Gui::BeginMenu("New")) {
            if(Gui::MenuItem("Entity")) {
                if(App::GetCurrentScene().Valid()) {
                    App::Entity@ entity = App::GetCurrentScene().Create("Entity");
                }
            }
            Gui::EndMenu();
        }
        Gui::EndContextMenu();
    }
}

void Console() {
    Gui::Text("Console");
}

void Properties() {
    Gui::Text("Properties");
}

void Filesystem() {
    Gui::DrawFilesystem();
}

void Game() {
    Gui::DrawPlayButton();
    Gui::Separator();
    Gui::DrawFrame();
}